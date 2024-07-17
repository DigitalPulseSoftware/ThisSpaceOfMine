// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstance.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <ServerLib/NetworkedEntitiesSystem.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <charconv>
#include <cstdio>
#include <memory>

namespace tsom
{
	constexpr unsigned int chunkSaveVersion = 1;

	ServerInstance::ServerInstance(Nz::ApplicationBase& application, Config config) :
	m_connectionTokenEncryptionKey(config.connectionTokenEncryptionKey),
	m_saveDirectory(std::move(config.saveDirectory)),
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_saveInterval(config.saveInterval),
	m_tickIndex(0),
	m_application(application),
	m_pauseWhenEmpty(config.pauseWhenEmpty)
	{
		m_world.AddSystem<NetworkedEntitiesSystem>(*this);
		auto& physicsSystem = m_world.AddSystem<Nz::Physics3DSystem>();
		{
			auto& physWorld = physicsSystem.GetPhysWorld();
			physWorld.SetStepSize(m_tickDuration);
			physWorld.SetGravity(Nz::Vector3f::Zero());

			m_world.AddSystem<PlanetGravitySystem>(physWorld);
		}

		auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();

		m_planet = std::make_unique<Planet>(1.f, 16.f, 9.81f);
		m_planet->GenerateChunks(m_blockLibrary, taskScheduler, config.planetSeed, config.planetChunkCount);
		LoadChunks();
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Right, { 65, -18, -39 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Back, { -34, 2, 53 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Front, { 22, -35, -59 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Down, { 23, -62, 26 });

		m_planet->OnChunkUpdated.Connect([this](ChunkContainer* /*planet*/, Chunk* chunk)
		{
			m_dirtyChunks.insert(chunk->GetIndices());
		});

		m_planetEntities = std::make_unique<ChunkEntities>(m_application, m_world, *m_planet, m_blockLibrary);
	}

	ServerInstance::~ServerInstance()
	{
		OnSave();

		m_sessionManagers.clear();
		m_players.Clear();
	}

	void ServerInstance::BroadcastChatMessage(std::string message, std::optional<PlayerIndex> senderIndex)
	{
		Packets::ChatMessage chatMessage;
		chatMessage.message = std::move(message);
		chatMessage.playerIndex = senderIndex;

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			if (NetworkSession* session = serverPlayer.GetSession())
				session->SendPacket(chatMessage);
		});
	}

	ServerPlayer* ServerInstance::CreateAnonymousPlayer(NetworkSession* session, std::string nickname)
	{
		// Check if a player already has this nickname and rename it if it's the case
		if (FindPlayerByNickname(nickname) != nullptr)
		{
			std::string newNickname;
			unsigned int counter = 2;
			do
			{
				newNickname = fmt::format("{}_{}", nickname, counter++);
			}
			while (FindPlayerByNickname(newNickname) != nullptr);

			nickname = std::move(newNickname);
		}

		std::size_t playerIndex;

		// defer construct so player can be constructed with their index
		ServerPlayer* player = m_players.Allocate(m_players.DeferConstruct, playerIndex);
		std::construct_at(player, *this, Nz::SafeCast<PlayerIndex>(playerIndex), session, std::nullopt, std::move(nickname));

		m_newPlayers.UnboundedSet(playerIndex);

		// Send all chunks (waiting for chunk streaming based on visibility)
		auto& playerVisibility = player->GetVisibilityHandler();
		m_planet->ForEachChunk([&](const ChunkIndices& chunkIndices, const Chunk& chunk)
		{
			playerVisibility.CreateChunk(chunk);
		});

		return player;
	}

	ServerPlayer* ServerInstance::CreateAuthenticatedPlayer(NetworkSession* session, const Nz::Uuid& uuid, std::string nickname)
	{
		// Disconnect an existing player if it exists with this uuid
		// TODO: Override the player session with this one
		if (ServerPlayer* player = FindPlayerByUuid(uuid))
			player->GetSession()->Disconnect(DisconnectionType::Kick);

		// Check if a player already has this nickname and rename it if it's the case
		if (ServerPlayer* player = FindPlayerByNickname(nickname))
		{
			std::string newNickname;
			unsigned int counter = 2;
			do
			{
				newNickname = fmt::format("{}_{}", nickname, counter++);
			} while (FindPlayerByNickname(newNickname) != nullptr);

			player->UpdateNickname(newNickname);
			m_pendingPlayerRename.push_back({ player->GetPlayerIndex(), std::move(newNickname) });
		}

		std::size_t playerIndex;

		// defer construct so player can be constructed with their index
		ServerPlayer* player = m_players.Allocate(m_players.DeferConstruct, playerIndex);
		std::construct_at(player, *this, Nz::SafeCast<PlayerIndex>(playerIndex), session, uuid, std::move(nickname));

		m_newPlayers.UnboundedSet(playerIndex);

		// Send all chunks (waiting for chunk streaming based on visibility)
		auto& playerVisibility = player->GetVisibilityHandler();
		m_planet->ForEachChunk([&](const ChunkIndices& chunkIndices, const Chunk& chunk)
		{
			playerVisibility.CreateChunk(chunk);
		});

		return player;
	}

	void ServerInstance::DestroyPlayer(PlayerIndex playerIndex)
	{
		ServerPlayer* player = m_players.RetrieveFromIndex(playerIndex);
		if (entt::handle controlledEntity = player->GetControlledEntity())
			controlledEntity.destroy();

		m_disconnectedPlayers.UnboundedSet(playerIndex);
		m_newPlayers.UnboundedReset(playerIndex);

		m_players.Free(playerIndex);
	}

	Nz::Time ServerInstance::Update(Nz::Time elapsedTime)
	{
		if (m_saveClock.RestartIfOver(m_saveInterval))
			OnSave();

		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		// No player? Pause instance for 100ms
		if (m_pauseWhenEmpty && m_players.begin() == m_players.end())
			return Nz::Time::Milliseconds(100);

		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			OnTick(m_tickDuration);
			m_tickAccumulator -= m_tickDuration;
		}

		return m_tickDuration - m_tickAccumulator;
	}

	void ServerInstance::LoadChunks()
	{
		if (!std::filesystem::is_directory(m_saveDirectory))
		{
			fmt::print("save directory {0} doesn't exist, not loading chunks\n", m_saveDirectory);
			return;
		}

		// Handle conversion
		unsigned int saveVersion = 0;
		if (std::filesystem::path versionPath = m_saveDirectory / Nz::Utf8Path("version.txt"); std::filesystem::exists(versionPath))
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
				fmt::print(stderr, fg(fmt::color::red), "failed to load planet: failed to load version file\n");
		}

		bool didConvert = false;
		if (saveVersion == 0)
		{
			std::filesystem::path oldSave = m_saveDirectory / Nz::Utf8Path("old0");
			std::filesystem::create_directory(oldSave);
			for (const auto& entry : std::filesystem::directory_iterator(m_saveDirectory))
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

				std::filesystem::rename(entry.path(), m_saveDirectory / newFilename);
			}

			saveVersion++;
			didConvert = true;
		}

		if (didConvert)
		{
			std::string version = std::to_string(saveVersion);
			Nz::File::WriteWhole(m_saveDirectory / Nz::Utf8Path("version.txt"), version.data(), version.size());
		}

		m_planet->ForEachChunk([&](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			Nz::File chunkFile(m_saveDirectory / Nz::Utf8Path(fmt::format("{0:+}_{1:+}_{2:+}.chunk", chunkIndices.x, chunkIndices.y, chunkIndices.z)), Nz::OpenMode::Read);
			if (!chunkFile.IsOpen())
				return;

			try
			{
				Nz::ByteStream fileStream(&chunkFile);
				chunk.Deserialize(m_blockLibrary, fileStream);
			}
			catch (const std::exception& e)
			{
				fmt::print(stderr, fg(fmt::color::red), "failed to load chunk {}: {}\n", fmt::streamed(chunkIndices), e.what());
			}
		});
	}

	void ServerInstance::OnNetworkTick()
	{
		// Handle disconnected players
		for (std::size_t playerIndex = m_disconnectedPlayers.FindFirst(); playerIndex != m_disconnectedPlayers.npos; playerIndex = m_disconnectedPlayers.FindNext(playerIndex))
		{
			Packets::PlayerLeave playerLeave;
			playerLeave.index = Nz::SafeCast<PlayerIndex>(playerIndex);

			ForEachPlayer([&](ServerPlayer& serverPlayer)
			{
				if (NetworkSession* session = serverPlayer.GetSession())
					session->SendPacket(playerLeave);
			});

			for (auto it = m_pendingPlayerRename.begin(); it != m_pendingPlayerRename.end();)
			{
				if (it->playerIndex == playerLeave.index)
					it = m_pendingPlayerRename.erase(it);
				else
					++it;
			}
		}
		m_disconnectedPlayers.Clear();

		// Handle renaming
		for (auto&& [playerIndex, newNickname] : m_pendingPlayerRename)
		{
			Packets::PlayerNameUpdate playerNameUpdate;
			playerNameUpdate.index = Nz::SafeCast<PlayerIndex>(playerIndex);
			playerNameUpdate.newNickname = std::move(newNickname);

			ForEachPlayer([&](ServerPlayer& serverPlayer)
			{
				if (NetworkSession* session = serverPlayer.GetSession())
					session->SendPacket(playerNameUpdate);
			});
		}
		m_pendingPlayerRename.clear();

		// Handle newly connected players
		for (std::size_t playerIndex = m_newPlayers.FindFirst(); playerIndex != m_newPlayers.npos; playerIndex = m_newPlayers.FindNext(playerIndex))
		{
			ServerPlayer* player = m_players.RetrieveFromIndex(playerIndex);

			// Send a packet to existing players telling them someone just arrived
			Packets::PlayerJoin playerJoined;
			playerJoined.index = Nz::SafeCast<PlayerIndex>(playerIndex);
			playerJoined.nickname = player->GetNickname();
			playerJoined.isAuthenticated = player->IsAuthenticated();

			ForEachPlayer([&](ServerPlayer& serverPlayer)
			{
				// Don't send this to player connecting
				if (m_newPlayers.UnboundedTest(serverPlayer.GetPlayerIndex()))
					return;

				if (NetworkSession* session = serverPlayer.GetSession())
					session->SendPacket(playerJoined);
			});

			// Send a packet to the new player containing all existing players
			if (NetworkSession* session = player->GetSession())
			{
				Packets::GameData gameData;
				gameData.tickIndex = m_tickIndex;

				ForEachPlayer([&](ServerPlayer& serverPlayer)
				{
					auto& playerData = gameData.players.emplace_back();
					playerData.index = Nz::SafeCast<PlayerIndex>(serverPlayer.GetPlayerIndex());
					playerData.nickname = serverPlayer.GetNickname();
					playerData.isAuthenticated = serverPlayer.IsAuthenticated();
				});

				session->SendPacket(gameData);
			}
		}
		m_newPlayers.Clear();

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.GetVisibilityHandler().Dispatch(m_tickIndex);
		});
	}

	void ServerInstance::OnTick(Nz::Time elapsedTime)
	{
		m_tickIndex++;

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.Tick();
		});

		m_planetEntities->Update();

		m_world.Update(elapsedTime);

		OnNetworkTick();
	}

	void ServerInstance::OnSave()
	{
		if (m_dirtyChunks.empty())
			return;

		fmt::print("saving {} dirty chunks...\n", m_dirtyChunks.size());

		if (!std::filesystem::is_directory(m_saveDirectory))
			std::filesystem::create_directories(m_saveDirectory);

		Nz::ByteArray byteArray;
		for (const ChunkIndices& chunkIndices : m_dirtyChunks)
		{
			byteArray.Clear();

			Nz::ByteStream byteStream(&byteArray);
			m_planet->GetChunk(chunkIndices)->Serialize(m_blockLibrary, byteStream);

			if (!Nz::File::WriteWhole(m_saveDirectory / Nz::Utf8Path(fmt::format("{0:+}_{1:+}_{2:+}.chunk", chunkIndices.x, chunkIndices.y, chunkIndices.z)), byteArray.GetBuffer(), byteArray.GetSize()))
				fmt::print(stderr, "failed to save chunk {}\n", fmt::streamed(chunkIndices));
		}
		m_dirtyChunks.clear();

		std::string version = std::to_string(chunkSaveVersion);
		Nz::File::WriteWhole(m_saveDirectory / Nz::Utf8Path("version.txt"), version.data(), version.size());
	}
}
