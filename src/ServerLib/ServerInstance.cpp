// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/NetworkedEntitiesSystem.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <Nazara/Utility/Components.hpp>
#include <fmt/format.h>
#include <fmt/std.h>
#include <memory>

namespace tsom
{
	ServerInstance::ServerInstance() :
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Nz::Time::TickDuration(30))
	{
		m_world.AddSystem<NetworkedEntitiesSystem>(*this);
		auto& physicsSystem = m_world.AddSystem<Nz::JoltPhysics3DSystem>();
		physicsSystem.GetPhysWorld().SetStepSize(m_tickDuration);
		physicsSystem.GetPhysWorld().SetGravity(Nz::Vector3f::Zero());

		m_planet = std::make_unique<Planet>(Nz::Vector3ui(160), 2.f, 16.f);
		m_planet->GenerateChunks();

		m_planetEntities = std::make_unique<PlanetEntities>(m_world, *m_planet);
	}

	ServerInstance::~ServerInstance()
	{
		m_sessionManagers.clear();
		m_players.Clear();
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

	void ServerInstance::UpdatePlanetBlock(const Nz::Vector3ui& chunkIndices, const Nz::Vector3ui& voxelIndices, VoxelBlock newBlock)
	{
		m_voxelGridUpdates.push_back({
			chunkIndices,
			voxelIndices,
			newBlock
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
				ForEachPlayer([&](ServerPlayer& serverPlayer)
				{
					Packets::PlayerJoin playerJoined;
					playerJoined.index = Nz::SafeCast<PlayerIndex>(serverPlayer.GetPlayerIndex());
					playerJoined.nickname = serverPlayer.GetNickname();

					session->SendPacket(playerJoined);
				});
			}

		}
		m_newPlayers.Clear();

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.GetVisibilityHandler().Dispatch();
		});
	}

	void ServerInstance::OnTick(Nz::Time elapsedTime)
	{
		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		if (!m_voxelGridUpdates.empty())
		{
			for (BlockUpdate& blockUpdate : m_voxelGridUpdates)
			{
				Chunk& chunk = m_planet->GetChunk(blockUpdate.chunkIndices);
				chunk.UpdateBlock(blockUpdate.voxelIndices, blockUpdate.newBlock);
			}

			{
				Nz::HighPrecisionClock colliderClock;
				m_planetEntities->Update();
				fmt::print("built collider in {}\n", fmt::streamed(colliderClock.GetElapsedTime()));
			}

			m_voxelGridUpdates.clear();
		}

		m_world.Update(elapsedTime);

		OnNetworkTick();
	}
}
