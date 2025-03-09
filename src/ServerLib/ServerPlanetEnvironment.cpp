// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Systems/BuoyancySystem.hpp>
#include <CommonLib/Systems/GravityPhysicsSystem.hpp>
#include <CommonLib/Systems/PlanetSystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/ByteArray.hpp>
#include <Nazara/Core/ByteStream.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	constexpr unsigned int s_chunkVersion = 1;

	ServerPlanetEnvironment::ServerPlanetEnvironment(ServerInstance& serverInstance, std::optional<Nz::UInt32> databaseId, std::string generatorName, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount, float cellSize, float cornerRadius) :
	ServerEnvironment(serverInstance, ServerEnvironmentType::Planet, true),
	m_databaseId(databaseId)
	{
		m_world->GetRegistry().ctx().emplace<ServerPlanetEnvironment*>(this);
		m_world->AddSystem<EnvironmentSwitchSystem>();

		auto& blockLibrary = serverInstance.GetBlockLibrary();

		m_planetEntity = CreateEntity();
		m_planetEntity.emplace<Nz::NodeComponent>();
		m_planetEntity.emplace<NetworkedComponent>();

		std::shared_ptr<const EntityClass> planetClass = serverInstance.GetEntityRegistry().FindClass("planet");

		auto& entityInstance = m_planetEntity.emplace<ClassInstanceComponent>(planetClass);
		entityInstance.UpdateProperty<EntityPropertyType::Float>("CellSize", cellSize);
		entityInstance.UpdateProperty<EntityPropertyType::Float>("CornerRadius", cornerRadius);
		entityInstance.UpdateProperty<EntityPropertyType::Float>("Gravity", 9.81f);

		planetClass->InitAndActivateEntity(m_planetEntity);

		auto& planetComponent = m_planetEntity.get<PlanetComponent>();
		planetComponent.planet->AddChunks(blockLibrary, chunkCount);

		if (m_databaseId)
			LoadFromDatabase();

		auto& app = serverInstance.GetApplication();
		auto& taskScheduler = app.GetComponent<Nz::TaskSchedulerAppComponent>();

		planetComponent.planet->GenerateChunks(blockLibrary, taskScheduler, seed, chunkCount, std::move(generatorName));
		taskScheduler.WaitForTasks();

		planetComponent.planet->GeneratePlatform(blockLibrary, Direction::Right, { 65, -18, -39 });
		planetComponent.planet->GeneratePlatform(blockLibrary, Direction::Back, { -34, 2, 53 });
		planetComponent.planet->GeneratePlatform(blockLibrary, Direction::Front, { 22, -35, -59 });
		planetComponent.planet->GeneratePlatform(blockLibrary, Direction::Down, { 23, -62, 26 });

		std::size_t emptyChunkCount = 0;
		planetComponent.planet->ForEachChunk([&](const ChunkIndices& /*osef*/, const Chunk& chunk)
		{
			emptyChunkCount += chunk.GetBlockCount(EmptyBlockIndex);
		});

		// We want the player to be able to breathe 5s per empty block count
		// the player breathe 100ml per second
		Nz::UInt64 oxygenAmount = Constants::SecondsToEmptyOxygenBlock * Nz::UInt64(Constants::PlayerOxygenConsumption) * emptyChunkCount;
		m_atmosphere.SetGasAmount(GasType::Oxygen, oxygenAmount);

		// We also want oxygen to be 21% of the atmosphere and have the rest as nitrogen
		m_atmosphere.SetGasAmount(GasType::Nitrogen, oxygenAmount * (100 - Constants::OxygenAtmospherePct) / Constants::OxygenAtmospherePct);

		planetComponent.planet->OnChunkUpdated.Connect([this](ChunkContainer* /*planet*/, Chunk* chunk, DirectionMask /*neighborMask*/, Nz::UInt32 /*layerMask*/)
		{
			m_dirtyChunks.insert(chunk->GetIndices());
		});

		auto& physicsSystem = m_world->GetSystem<Nz::Physics3DSystem>();
		m_world->AddSystem<BuoyancySystem>(*planetComponent.planet, physicsSystem.GetPhysWorld(), m_debugDrawer.get());
		m_world->AddSystem<GravityPhysicsSystem>(*planetComponent.planet, physicsSystem.GetPhysWorld());
		m_world->AddSystem<PlanetSystem>();
	}

	ServerPlanetEnvironment::~ServerPlanetEnvironment()
	{
		ClearEntities();

		m_world->GetRegistry().ctx().erase<ServerPlanetEnvironment*>();
	}

	Nz::Boxf ServerPlanetEnvironment::ComputeBoundingBox() const
	{
		Nz::Boxf boundingBox = Nz::Boxf::Invalid();
		auto& planet = *m_planetEntity.get<PlanetComponent>().planet;
		planet.ForEachChunk([&](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			Nz::Boxf chunkBox(Nz::Vector3f(chunk.GetBlockSize() * Planet::ChunkSize));
			chunkBox.Translate(planet.GetChunkOffset(chunkIndices));

			if (boundingBox.IsValid())
				boundingBox.ExtendTo(chunkBox);
			else
				boundingBox = chunkBox;
		});

		return boundingBox;
	}

	entt::handle ServerPlanetEnvironment::CreateEntity()
	{
		return ServerEnvironment::CreateEntity();
	}

	void ServerPlanetEnvironment::ForEachAtmosphere(Nz::FunctionRef<void(ServerAtmosphere*)> callback)
	{
		ServerEnvironment::ForEachAtmosphere(callback);

		callback(&m_atmosphere);
	}

	void ServerPlanetEnvironment::ForEachAtmosphere(Nz::FunctionRef<void(const ServerAtmosphere*)> callback) const
	{
		ServerEnvironment::ForEachAtmosphere(callback);

		callback(&m_atmosphere);
	}

	const GravityController* ServerPlanetEnvironment::GetGravityController() const
	{
		return m_planetEntity.get<PlanetComponent>().planet.get();
	}

	Planet& ServerPlanetEnvironment::GetPlanet()
	{
		return *m_planetEntity.get<PlanetComponent>().planet;
	}

	const Planet& ServerPlanetEnvironment::GetPlanet() const
	{
		return *m_planetEntity.get<PlanetComponent>().planet;
	}

	void ServerPlanetEnvironment::OnSave()
	{
		if (m_dirtyChunks.empty() || !m_databaseId)
			return;

		fmt::print("saving {} dirty chunks...\n", m_dirtyChunks.size());

		BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();
		ServerDatabase& serverDatabase = m_serverInstance.GetServerDatabase();
		const Planet& planet = GetPlanet();

		Nz::ByteArray byteArray;
		for (const ChunkIndices& chunkIndices : m_dirtyChunks)
		{
			byteArray.Clear();

			Nz::ByteStream byteStream(&byteArray);
			planet.GetChunk(chunkIndices)->Serialize(byteStream);

			Nz::UInt32 decompressedSize = Nz::SafeCaster(byteArray.GetSize());

			std::optional compressedDataOpt = binaryCompressor.Compress(byteArray.GetBuffer(), byteArray.GetSize());
			if NAZARA_UNLIKELY(!compressedDataOpt)
				throw std::runtime_error("chunk compression failed");

			std::span<Nz::UInt8>& compressedData = *compressedDataOpt;

			// Reuse byteArray
			byteArray.Clear();
			byteArray.Resize(sizeof(Nz::UInt32) + compressedData.size());

			decompressedSize = Nz::HostToLittleEndian(decompressedSize);
			std::memcpy(&byteArray[0], &decompressedSize, sizeof(decompressedSize));
			std::memcpy(&byteArray[sizeof(decompressedSize)], &compressedData[0], compressedData.size());

			serverDatabase.StorePlanetChunk(Database::PlanetChunk{
				.planetId = *m_databaseId,
				.position = chunkIndices,
				.version = s_chunkVersion,
				.chunkData = std::span(byteArray.GetBuffer(), byteArray.GetSize())
			});
		}
		m_dirtyChunks.clear();
	}

	ServerAtmosphere* ServerPlanetEnvironment::GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position)
	{
		Planet& planet = GetPlanet();
		if (position.SquaredDistance(planet.GetCenter()) > Nz::IntegralPow(100, 2))
			return nullptr; //< too far away

		return &m_atmosphere;
	}

	void ServerPlanetEnvironment::LoadFromDatabase()
	{
		NazaraAssert(m_databaseId);

		ServerDatabase& serverDatabase = m_serverInstance.GetServerDatabase();

		BinaryCompressor& binaryCompressor = BinaryCompressor::GetThreadCompressor();
		auto& blockLibrary = m_serverInstance.GetBlockLibrary();
		Planet& planet = GetPlanet();

		serverDatabase.GetPlanetChunks(*m_databaseId, [&](Database::PlanetChunk&& planetChunk)
		{
			if (planetChunk.version != s_chunkVersion)
				throw std::runtime_error(fmt::format("unhandled version {}", planetChunk.version));

			// Chunk data has decompressedSize first
			Nz::UInt32 decompressedSize;
			std::memcpy(&decompressedSize, &planetChunk.chunkData[0], sizeof(decompressedSize));
			decompressedSize = Nz::LittleEndianToHost(decompressedSize);

			std::vector<Nz::UInt8> decompressedData(decompressedSize);
			std::optional compressedDataOpt = binaryCompressor.Decompress(planetChunk.chunkData.data() + sizeof(decompressedSize), planetChunk.chunkData.size() - sizeof(decompressedSize), decompressedData.data(), decompressedData.size());
			if (!compressedDataOpt)
				throw std::runtime_error("chunk decompression failed");

			if (*compressedDataOpt != decompressedSize)
				throw std::runtime_error("chunk decompression failed (corrupt size)");

			Nz::ByteStream byteStream(decompressedData.data(), decompressedData.size());
			Chunk* chunk = planet.GetChunk(planetChunk.position);
			if (!chunk)
				chunk = &planet.AddChunk(blockLibrary, planetChunk.position);

			chunk->Deserialize(byteStream);

			return true;
		});
	}
}
