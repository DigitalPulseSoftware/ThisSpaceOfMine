// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstance.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Entities/ChunkClassLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/SharedScriptingLibrary.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Entities/ServerClassLibrary.hpp>
#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <memory>

namespace tsom
{
	ServerInstance::ServerInstance(Nz::ApplicationBase& application, Config config) :
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_tickIndex(0),
	m_application(application),
	m_config(std::move(config)),
	m_scriptingContext(application),
	m_serverDatabase(application, m_config.databaseFile)
	{
		m_entityRegistry.RegisterClassLibrary<ChunkClassLibrary>(m_application, m_blockLibrary);
		m_entityRegistry.RegisterClassLibrary<ServerClassLibrary>(m_application);

		m_scriptingContext.RegisterLibrary<MathScriptingLibrary>();
		auto& entityScriptingLibrary = m_scriptingContext.RegisterLibrary<ServerEntityScriptingLibrary>(m_entityRegistry);
		m_scriptingContext.RegisterLibrary<SharedScriptingLibrary>(entityScriptingLibrary);
		m_scriptingContext.RegisterLibrary<ServerScriptingLibrary>(*this, entityScriptingLibrary);

		LoadScripts();
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
		if (!m_defaultSpawnpoint.env)
		{
			fmt::print(fg(fmt::color::red), "cannot create player: no spawnpoint set\n");
			return nullptr;
		}

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
		std::construct_at(player, *this, Nz::SafeCast<PlayerIndex>(playerIndex), session, std::nullopt, std::move(nickname), 0);

		player->UpdateRootEnvironment(m_defaultSpawnpoint.env);
		player->Respawn(m_defaultSpawnpoint.env, m_defaultSpawnpoint.position, m_defaultSpawnpoint.rotation);

		m_newPlayers.UnboundedSet(playerIndex);

		return player;
	}

	ServerPlayer* ServerInstance::CreateAuthenticatedPlayer(NetworkSession* session, const Nz::Uuid& uuid, std::string nickname, PlayerPermissionFlags permissions)
	{
		if (!m_defaultSpawnpoint.env)
		{
			fmt::print(fg(fmt::color::red), "cannot create player: no spawnpoint set\n");
			return nullptr;
		}

		// Disconnect an existing player if it exists with this uuid
		// TODO: Override the player session with this one
		if (ServerPlayer* player = FindPlayerByUuid(uuid))
			player->GetSession()->Disconnect(DisconnectionType::Kick);
		else
		{
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
		}

		std::size_t playerIndex;

		// defer construct so player can be constructed with their index
		ServerPlayer* player = m_players.Allocate(m_players.DeferConstruct, playerIndex);
		std::construct_at(player, *this, Nz::SafeCast<PlayerIndex>(playerIndex), session, uuid, std::move(nickname), permissions);

		player->UpdateRootEnvironment(m_defaultSpawnpoint.env);
		player->Respawn(m_defaultSpawnpoint.env, m_defaultSpawnpoint.position, m_defaultSpawnpoint.rotation);

		m_newPlayers.UnboundedSet(playerIndex);

		return player;
	}

	void ServerInstance::DestroyPlayer(PlayerIndex playerIndex)
	{
		ServerPlayer* player = m_players.RetrieveFromIndex(playerIndex);

		m_disconnectedPlayers.UnboundedSet(playerIndex);
		m_newPlayers.UnboundedReset(playerIndex);

		m_players.Free(playerIndex);
	}

	void ServerInstance::LoadFromDatabase()
	{
		m_databaseEnvironments.clear();
		m_serverDatabase.GetAllPlanets([&](Database::Planet&& planetData)
		{
			auto planetEnv = std::make_unique<ServerPlanetEnvironment>(*this, planetData.id, std::string(planetData.generatorName), planetData.seed, planetData.chunkCount, 1.f, planetData.cornerRadius);
			SetDefaultSpawnpoint(planetEnv.get(), Nz::Vector3f::Up() * 100.f + Nz::Vector3f::Backward() * 5.f, Nz::Quaternionf::Identity());
			m_databaseEnvironments[planetData.id] = std::move(planetEnv);

			return true;
		});

		m_serverDatabase.GetAllPlanetLinks([&](Database::PlanetLink&& planetLink)
		{
			auto sourceIt = m_databaseEnvironments.find(planetLink.sourcePlanet);
			if (sourceIt == m_databaseEnvironments.end())
			{
				fmt::print(fg(fmt::color::red), "Loading database: planet_link entry references unknown planet {}\n", planetLink.sourcePlanet);
				return true;
			}

			auto destinationIt = m_databaseEnvironments.find(planetLink.destinationPlanet);
			if (destinationIt == m_databaseEnvironments.end())
			{
				fmt::print(fg(fmt::color::red), "Loading database: planet_link entry references unknown planet {}\n", planetLink.destinationPlanet);
				return true;
			}

			ServerEnvironment& sourceEnvironment = *sourceIt->second;
			ServerEnvironment& destinationEnvironment = *destinationIt->second;

			entt::handle switchTriggerEntity = sourceEnvironment.CreateEntity();
			switchTriggerEntity.emplace<Nz::NodeComponent>(planetLink.position);
			switchTriggerEntity.emplace<EnvironmentProxyComponent>().targetEnvironment = &destinationEnvironment;
			switchTriggerEntity.emplace<NetworkedComponent>();

			auto& enterTrigger = switchTriggerEntity.emplace<EnvironmentEnterTriggerComponent>();
			enterTrigger.aabb = destinationEnvironment.ComputeBoundingBox().ScaleAroundCenter(2.f);
			enterTrigger.targetEnvironment = &destinationEnvironment;
			enterTrigger.updateRoot = true;
			return true;
		});
	}

	std::unique_ptr<Nz::EnttWorld> ServerInstance::RegisterEnvironment(ServerEnvironment* environment)
	{
		assert(std::find(m_environments.begin(), m_environments.end(), environment) == m_environments.end());
		m_environments.push_back(environment);

		if (!m_envWorldPool.empty())
		{
			std::unique_ptr<Nz::EnttWorld> world = std::move(m_envWorldPool.back());
			m_envWorldPool.pop_back();

			return world;
		}
		else
			return std::make_unique<Nz::EnttWorld>();
	}

	void ServerInstance::UnregisterEnvironment(ServerEnvironment* environment, std::unique_ptr<Nz::EnttWorld>&& world)
	{
		auto it = std::find(m_environments.begin(), m_environments.end(), environment);
		assert(it != m_environments.end());
		m_environments.erase(it);

		m_envWorldPool.push_back(std::move(world));
	}

	Nz::Time ServerInstance::Update(Nz::Time elapsedTime)
	{
		if (m_saveClock.RestartIfOver(m_config.saveInterval))
			OnSave();

		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		// No player? Pause instance for 100ms
		if (m_config.pauseWhenEmpty && m_players.begin() == m_players.end())
			return Nz::Time::Milliseconds(100);

		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			OnTick(m_tickDuration);
			m_tickAccumulator -= m_tickDuration;
		}

		return m_tickDuration - m_tickAccumulator;
	}

	void ServerInstance::LoadScripts(bool isReloading)
	{
		if (!isReloading)
		{
			m_scriptingContext.LoadDirectory("scripts/entities");
			return;
		}

		std::vector<entt::registry*> registries;
		for (ServerEnvironment* environment : m_environments)
			registries.push_back(&environment->GetWorld().GetRegistry());

		m_entityRegistry.Refresh(registries, [this]
		{
			LoadScripts(false);
		});
	}

	void ServerInstance::OnNetworkTick()
	{
		// Handle disconnected players
		for (std::size_t playerIndex : m_disconnectedPlayers.IterBits())
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
		for (std::size_t playerIndex : m_newPlayers.IterBits())
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

	void ServerInstance::OnSave()
	{
		for (ServerEnvironment* env : m_environments)
			env->OnSave();
	}

	void ServerInstance::OnTick(Nz::Time elapsedTime)
	{
		m_tickIndex++;
		m_tickedTimerManager.Update(elapsedTime);

		// Use two lists to avoid reallocation issues if a callback were to call ScheduleForNextTick itself
		std::swap(m_scheduledTickFunctions, m_nextScheduledTickFunctions);
		for (auto& callback : m_nextScheduledTickFunctions)
			callback();
		m_nextScheduledTickFunctions.clear();

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.Tick();
		});

		for (ServerEnvironment* env : m_environments)
			env->OnTick(elapsedTime);

		OnNetworkTick();
	}
}
