// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstance.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Entities/ChunkClassLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/File.hpp>
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
	ServerInstance::ServerInstance(Nz::ApplicationBase& application, Config config) :
	m_connectionTokenEncryptionKey(config.connectionTokenEncryptionKey),
	m_players(256),
	m_saveInterval(config.saveInterval),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_tickIndex(0),
	m_application(application),
	m_scriptingContext(application),
	m_pauseWhenEmpty(config.pauseWhenEmpty)
	{
		m_entityRegistry.RegisterClassLibrary<ChunkClassLibrary>(m_application, m_blockLibrary);

		m_scriptingContext.RegisterLibrary<MathScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<ServerEntityScriptingLibrary>(m_entityRegistry);
		m_scriptingContext.RegisterLibrary<ServerScriptingLibrary>(m_application);
		m_scriptingContext.LoadDirectory("scripts/entities");
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

	void ServerInstance::RegisterEnvironment(ServerEnvironment* environment)
	{
		assert(std::find(m_environments.begin(), m_environments.end(), environment) == m_environments.end());
		m_environments.push_back(environment);
	}

	void ServerInstance::UnregisterEnvironment(ServerEnvironment* environment)
	{
		auto it = std::find(m_environments.begin(), m_environments.end(), environment);
		assert(it != m_environments.end());
		m_environments.erase(it);
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

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.Tick();
		});

		for (ServerEnvironment* env : m_environments)
			env->OnTick(elapsedTime);

		OnNetworkTick();
	}
}
