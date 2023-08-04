// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ServerInstance.hpp>
#include <CommonLib/NetworkedEntitiesSystem.hpp>
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

		m_planet = std::make_unique<Planet>(Nz::Vector3ui(80), 2.f, 2.f);

		m_planetEntity = m_world.CreateEntity();
		{
			m_planetEntity.emplace<Nz::NodeComponent>();

			Nz::JoltRigidBody3D::StaticSettings settings;
			{
				Nz::HighPrecisionClock colliderClock;
				settings.geom = m_planet->BuildCollider();
				fmt::print("built collider in {}\n", fmt::streamed(colliderClock.GetElapsedTime()));
			}

			m_planetEntity.emplace<Nz::JoltRigidBody3DComponent>(settings);
		}
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

		return player;
	}

	void ServerInstance::DestroyPlayer(PlayerIndex playerIndex)
	{
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

	void ServerInstance::UpdatePlanetBlock(const Nz::Vector3f& position, VoxelBlock newBlock)
	{
		m_voxelGridUpdates.push_back({
			position,
			newBlock
		});
	}

	void ServerInstance::OnNetworkTick()
	{
		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

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

				// Send planet (TEMP)
				if (m_voxelGridUpdateLastSize > 0)
				{
					Packets::VoxelGridUpdate voxelGridUpdatePacket;

					for (std::size_t i = 0; i < m_voxelGridUpdates.size(); ++i)
					{
						auto&& [pos, block] = m_voxelGridUpdates[i];
						if (auto intersectionData = m_planet->ComputeGridCell(pos))
						{
							voxelGridUpdatePacket.updates.push_back({
								pos,
								Nz::SafeCast<Nz::UInt8>(block)
								});
						}
					}

					session->SendPacket(voxelGridUpdatePacket);
				}
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
		OnNetworkTick();

		if (m_voxelGridUpdates.size() > m_voxelGridUpdateLastSize)
		{
			Packets::VoxelGridUpdate voxelGridUpdatePacket;

			for (std::size_t i = m_voxelGridUpdateLastSize; i < m_voxelGridUpdates.size(); ++i)
			{
				auto&& [pos, block] = m_voxelGridUpdates[i];
				if (auto intersectionData = m_planet->ComputeGridCell(pos))
				{
					//m_planet->GetChunk().UpdateCell(intersectionData->x, intersectionData->y, intersectionData->z, block);

					voxelGridUpdatePacket.updates.push_back({
						pos,
						Nz::SafeCast<Nz::UInt8>(block)
					});
				}
			}

			m_voxelGridUpdateLastSize = m_voxelGridUpdates.size();


			std::shared_ptr<Nz::JoltCollider3D> geom;
			{
				Nz::HighPrecisionClock colliderClock;
				geom = m_planet->BuildCollider();
				fmt::print("built collider in {}\n", fmt::streamed(colliderClock.GetElapsedTime()));
			}

			auto& planetBody = m_planetEntity.get<Nz::JoltRigidBody3DComponent>();
			planetBody.SetGeom(geom, false);

			ForEachPlayer([&](ServerPlayer& serverPlayer)
			{
				serverPlayer.GetSession()->SendPacket(voxelGridUpdatePacket);
			});
		}

		m_world.Update(elapsedTime);
	}
}
