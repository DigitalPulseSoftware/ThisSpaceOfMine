// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstance.hpp>
#include <CommonLib/GameConstants.hpp>
#include <ServerLib/NetworkedEntitiesSystem.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <Nazara/Utility/Components.hpp>
#include <fmt/format.h>
#include <fmt/std.h>
#include <memory>

namespace tsom
{
	ServerInstance::ServerInstance() :
	m_tickIndex(0),
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Constants::TickDuration),
	m_gravitySystem(m_world)
	{
		m_world.AddSystem<NetworkedEntitiesSystem>(*this);
		auto& physicsSystem = m_world.AddSystem<Nz::JoltPhysics3DSystem>();
		physicsSystem.GetPhysWorld().SetStepSize(m_tickDuration);
		physicsSystem.GetPhysWorld().SetGravity(Nz::Vector3f::Zero());
		physicsSystem.GetPhysWorld().RegisterStepListener(&m_gravitySystem);

		m_planet = std::make_unique<Planet>(Nz::Vector3ui(160), 1.f, 16.f, 9.81f);
		m_planet->GenerateChunks(m_blockLibrary);
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Up, { 68, 109, 150 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Back, { 33, 148, 60 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Front, { 50, 12, 63 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Left, { 8, 87, 111 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Right, { 149, 73, 100 });
		m_planet->GeneratePlatform(m_blockLibrary, tsom::Direction::Down, { 91, 92, 2 });

		m_planetEntities = std::make_unique<ChunkEntities>(m_world, *m_planet, m_blockLibrary);
	}

	ServerInstance::~ServerInstance()
	{
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

		m_newPlayers.UnboundedSet(playerIndex);

		// Send all chunks
		auto& playerVisibility = player->GetVisibilityHandler();

		std::size_t chunkCount = m_planet->GetChunkCount();
		for (std::size_t i = 0; i < chunkCount; ++i)
		{
			if (Chunk* chunk = m_planet->GetChunk(i))
				playerVisibility.CreateChunk(*chunk);
		}

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

	void ServerInstance::Update(Nz::Time elapsedTime)
	{
		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			OnTick(m_tickDuration);
			m_tickAccumulator -= m_tickDuration;
		}
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

	void ServerInstance::OnTick(Nz::Time elapsedTime)
	{
		m_tickIndex++;

		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.Tick();
		});

		if (m_planetEntities->DoesRequireUpdate())
		{
			Nz::HighPrecisionClock colliderClock;
			m_planetEntities->Update();
			fmt::print("updated planet collider in {}\n", fmt::streamed(colliderClock.GetElapsedTime()));
		}

		m_world.Update(elapsedTime);

		OnNetworkTick();
	}
}
