// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/ByteArray.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace tsom
{
	constexpr unsigned int chunkSaveVersion = 1;

	ServerPlanetEnvironment::ServerPlanetEnvironment(ServerInstance& serverInstance, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount) :
	ServerEnvironment(serverInstance)
	{
		auto& app = serverInstance.GetApplication();
		auto& taskScheduler = app.GetComponent<Nz::TaskSchedulerAppComponent>();

		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_planet = std::make_unique<Planet>(1.f, 16.f, 9.81f);
		m_planet->GenerateChunks(blockLibrary, taskScheduler.GetScheduler(), seed, chunkCount);
		m_planet->GeneratePlatform(blockLibrary, tsom::Direction::Right, { 65, -18, -39 });
		m_planet->GeneratePlatform(blockLibrary, tsom::Direction::Back, { -34, 2, 53 });
		m_planet->GeneratePlatform(blockLibrary, tsom::Direction::Front, { 22, -35, -59 });
		m_planet->GeneratePlatform(blockLibrary, tsom::Direction::Down, { 23, -62, 26 });

		m_planet->OnChunkUpdated.Connect([this](ChunkContainer* /*planet*/, Chunk* chunk)
		{
			m_dirtyChunks.insert(chunk->GetIndices());
		});

		m_planetEntities = std::make_unique<ChunkEntities>(app, m_world, *m_planet, blockLibrary);

		auto& physicsSystem = m_world.GetSystem<Nz::Physics3DSystem>();
		m_world.AddSystem<PlanetGravitySystem>(physicsSystem.GetPhysWorld());
	}

	ServerPlanetEnvironment::~ServerPlanetEnvironment() = default;

	void ServerPlanetEnvironment::OnLoad(const std::filesystem::path& loadPath)
	{
		if (!std::filesystem::is_directory(loadPath))
		{
			fmt::print("save directory {0} doesn't exist, not loading chunks\n", loadPath);
			return;
		}

		// Handle conversion
		unsigned int saveVersion = 0;
		if (std::filesystem::path versionPath = loadPath / Nz::Utf8Path("version.txt"); std::filesystem::exists(versionPath))
		{
			auto contentOpt = Nz::File::ReadWhole(versionPath);
			if (contentOpt)
			{
				const char* ptr = reinterpret_cast<const char*>(contentOpt->data());
				if (auto err = std::from_chars(ptr, ptr + contentOpt->size(), saveVersion); err.ec != std::errc{})
				{
					fmt::print(stderr, fg(fmt::color::red), "failed to load planet: invalid version file (not a number)\n");
					return;
				}

				if (saveVersion > chunkSaveVersion)
				{
					fmt::print(stderr, fg(fmt::color::red), "failed to load planet: unknown save version {0}\n", saveVersion);
					return;
				}
			}
			else
				fmt::print(stderr, fg(fmt::color::red), "failed to load planet: failed to load version file {}\n");
		}

		bool didConvert = false;
		if (saveVersion == 0)
		{
			std::filesystem::path oldSave = loadPath / Nz::Utf8Path("old0");
			std::filesystem::create_directory(oldSave);
			for (const auto& entry : std::filesystem::directory_iterator(loadPath))
			{
				if (!entry.is_regular_file())
					continue;

				if (entry.path().extension() != Nz::Utf8Path(".chunk"))
					continue;

				std::filesystem::copy_file(entry.path(), oldSave / entry.path().filename());

				std::string fileName = Nz::PathToString(entry.path().filename());
				unsigned int x, y, z;
				if (std::sscanf(fileName.c_str(), "%u_%u_%u.chunk", &x, &y, &z) != 3)
				{
					fmt::print(stderr, fg(fmt::color::red), "planet conversion: failed to parse chunk name {}\n", fileName);
					continue;
				}

				ChunkIndices chunkIndices(Nz::Vector3ui(x, y, z));
				chunkIndices -= Nz::Vector3i(3); // previous planet had 6x6x6 chunks

				std::filesystem::path newFilename = Nz::Utf8Path(fmt::format("{0:+}_{1:+}_{2:+}.chunk", chunkIndices.x, chunkIndices.z, chunkIndices.y)); //< reverse y & z

				std::filesystem::rename(entry.path(), loadPath / newFilename);
			}

			saveVersion++;
			didConvert = true;
		}

		if (didConvert)
		{
			std::string version = std::to_string(saveVersion);
			Nz::File::WriteWhole(loadPath / Nz::Utf8Path("version.txt"), version.data(), version.size());
		}

		m_planet->ForEachChunk([&](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			Nz::File chunkFile(loadPath / Nz::Utf8Path(fmt::format("{0:+}_{1:+}_{2:+}.chunk", chunkIndices.x, chunkIndices.y, chunkIndices.z)), Nz::OpenMode::Read);
			if (!chunkFile.IsOpen())
				return;

			try
			{
				Nz::ByteStream fileStream(&chunkFile);
				chunk.Unserialize(m_serverInstance.GetBlockLibrary(), fileStream);
			}
			catch (const std::exception& e)
			{
				fmt::print(stderr, fg(fmt::color::red), "failed to load chunk {}: {}\n", fmt::streamed(chunkIndices), e.what());
			}
		});
	}

	void ServerPlanetEnvironment::OnSave(const std::filesystem::path& savePath)
	{
		if (m_dirtyChunks.empty())
			return;

		fmt::print("saving {} dirty chunks...\n", m_dirtyChunks.size());

		if (!std::filesystem::is_directory(savePath))
			std::filesystem::create_directories(savePath);

		Nz::ByteArray byteArray;
		for (const ChunkIndices& chunkIndices : m_dirtyChunks)
		{
			byteArray.Clear();

			Nz::ByteStream byteStream(&byteArray);
			m_planet->GetChunk(chunkIndices)->Serialize(m_serverInstance.GetBlockLibrary(), byteStream);

			if (!Nz::File::WriteWhole(savePath / Nz::Utf8Path(fmt::format("{0:+}_{1:+}_{2:+}.chunk", chunkIndices.x, chunkIndices.y, chunkIndices.z)), byteArray.GetBuffer(), byteArray.GetSize()))
				fmt::print(stderr, "failed to save chunk {}\n", fmt::streamed(chunkIndices));
		}
		m_dirtyChunks.clear();

		std::string version = std::to_string(chunkSaveVersion);
		Nz::File::WriteWhole(savePath / Nz::Utf8Path("version.txt"), version.data(), version.size());
	}

	void ServerPlanetEnvironment::OnTick(Nz::Time elapsedTime)
	{
		ServerEnvironment::OnTick(elapsedTime);

		m_planetEntities->Update();
	}
}
