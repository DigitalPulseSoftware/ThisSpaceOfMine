// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstance.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Planet.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
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
	ServerInstance::ServerInstance(Nz::ApplicationBase& application, Config config) :
	m_saveDirectory(std::move(config.saveDirectory)),
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_tickIndex(0),
	m_application(application)
	{
		m_planetEnvironment = std::make_unique<ServerPlanetEnvironment>(*this, config.planetSeed, config.planetChunkCount);
		m_planetEnvironment->OnLoad(m_saveDirectory);
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

	ServerPlayer* ServerInstance::CreatePlayer(NetworkSession* session, std::string nickname)
	{
		std::size_t playerIndex;

		// defer construct so player can be constructed with their index
		ServerPlayer* player = m_players.Allocate(m_players.DeferConstruct, playerIndex);
		std::construct_at(player, *this, Nz::SafeCast<PlayerIndex>(playerIndex), session, std::move(nickname));

		player->UpdateEnvironment(m_planetEnvironment.get());

		m_newPlayers.UnboundedSet(playerIndex);

		// Send all chunks
		auto& playerVisibility = player->GetVisibilityHandler();
		m_planetEnvironment->GetPlanet().ForEachChunk([&](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			playerVisibility.CreateChunk(chunk);
		});

		return player;
	}

	ServerShipEnvironment* ServerInstance::CreateShip()
	{
		auto shipEnv = std::make_unique<ServerShipEnvironment>(*this);
		ServerShipEnvironment* shipEnvPtr = shipEnv.get();
		m_shipEnvironments.emplace_back(std::move(shipEnv));

		return shipEnvPtr;
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

	void ServerInstance::DestroyShip(ServerShipEnvironment* ship)
	{
		auto it = std::find_if(m_shipEnvironments.begin(), m_shipEnvironments.end(), [ship](auto& shipEnvPtr)
		{
			return shipEnvPtr.get() == ship;
		});

		assert(it != m_shipEnvironments.end());
		m_shipEnvironments.erase(it);
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
		for (std::size_t playerIndex = m_disconnectedPlayers.FindFirst(); playerIndex != m_disconnectedPlayers.npos; playerIndex = m_disconnectedPlayers.FindNext(playerIndex))
		{
			Packets::PlayerLeave playerLeave;
			playerLeave.index = Nz::SafeCast<PlayerIndex>(playerIndex);

			ForEachPlayer([&](ServerPlayer& serverPlayer)
			{
				if (NetworkSession* session = serverPlayer.GetSession())
					session->SendPacket(playerLeave);
			});
		}
		m_disconnectedPlayers.Clear();

		// Handle newly connected players
		for (std::size_t playerIndex = m_newPlayers.FindFirst(); playerIndex != m_newPlayers.npos; playerIndex = m_newPlayers.FindNext(playerIndex))
		{
			ServerPlayer* player = m_players.RetrieveFromIndex(playerIndex);

			// Send a packet to existing players telling them someone just arrived
			Packets::PlayerJoin playerJoined;
			playerJoined.index = Nz::SafeCast<PlayerIndex>(playerIndex);
			playerJoined.nickname = player->GetNickname();

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
		m_planetEnvironment->OnSave(m_saveDirectory);
	}

	void ServerInstance::OnTick(Nz::Time elapsedTime)
	{
		m_tickIndex++;

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.Tick();
		});

		m_planetEnvironment->OnTick(elapsedTime);
		for (auto& shipEnv : m_shipEnvironments)
			shipEnv->OnTick(elapsedTime);

		OnNetworkTick();
	}
}
