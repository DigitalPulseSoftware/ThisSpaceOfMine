// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/PhysCharacter3DComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	NetworkedEntitiesSystem::NetworkedEntitiesSystem(entt::registry& registry, ServerEnvironment& environment) :
	m_networkedEntities(registry),
	m_registry(registry),
	m_environment(environment),
	m_players(environment.GetServerInstance())
	{
		m_networkedEntities.OnEntityAdded.Connect([&](entt::entity entity)
		{
			// Handle entities in Update to let all components initialize themselves
			m_pendingCreatedEntities.emplace(entity);
		});

		m_networkedEntities.OnEntityRemove.Connect([&](entt::entity entity)
		{
			if (!m_pendingCreatedEntities.remove(entity))
			{
				ForEachVisibility([&](SessionVisibilityHandler& visibility)
				{
					visibility.DestroyEntity(entt::handle(m_registry, entity));
				});
			}
		});
	}

	void NetworkedEntitiesSystem::ForEachVisibility(const Nz::FunctionRef<void(SessionVisibilityHandler& visibility)>& functor)
	{
		m_players.ForEachPlayer([&](ServerPlayer& player)
		{
			functor(player.GetVisibilityHandler());
		});
	}

	bool NetworkedEntitiesSystem::ForgetEntity(entt::entity entity)
	{
		m_networkedEntities.Remove(entity);
		return m_pendingCreatedEntities.remove(entity);
	}

	void NetworkedEntitiesSystem::UnregisterPlayer(ServerPlayer* player)
	{
		auto it = std::find(m_pendingPlayers.begin(), m_pendingPlayers.end(), player);
		if (it != m_pendingPlayers.end())
			m_pendingPlayers.erase(it);
		else
			m_players.UnregisterPlayer(player);
	}

	void NetworkedEntitiesSystem::Update(Nz::Time /*elapsedTime*/)
	{
		for (entt::entity entity : m_pendingCreatedEntities)
		{
			EntityData& entityData = m_networkedEntities.Get(entity);

			if (ClassInstanceComponent* entityInstance = m_registry.try_get<ClassInstanceComponent>(entity))
			{
				entityData.onClientRpc.Connect(entityInstance->OnClientRpc, [this, entity](ClassInstanceComponent* emitter, Nz::UInt32 rpcIndex, ServerPlayer* targetPlayer)
				{
					entt::handle handle(m_registry, entity);
					if (targetPlayer)
						targetPlayer->GetVisibilityHandler().TriggerEntityRpc(handle, rpcIndex);
					else
					{
						ForEachVisibility([&](SessionVisibilityHandler& visibility)
						{
							visibility.TriggerEntityRpc(handle, rpcIndex);
						});
					}
				});

				entityData.onPropertyUpdate.Connect(entityInstance->OnPropertyUpdate, [this, entity](ClassInstanceComponent* emitter, Nz::UInt32 propertyIndex, const EntityProperty& newValue)
				{
					if (!emitter->GetClass()->GetProperty(propertyIndex).isNetworked)
						return;

					entt::handle handle(m_registry, entity);
					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.UpdateEntityProperty(handle, propertyIndex, newValue);
					});
				});
			}

			if (DistributionComponent* entityDistribution = m_registry.try_get<DistributionComponent>(entity))
			{
				entityData.onOutputChanged.Connect(entityDistribution->OnOutputChanged, [this, entity](DistributionComponent* distribution, std::size_t outputIndex)
				{
					entt::handle handle(m_registry, entity);
					Nz::UInt8 outputIndex8 = Nz::SafeCaster(outputIndex);
					Nz::UInt8 inputIndex8 = Nz::SafeCaster(distribution->GetOutputConnectedPort(outputIndex));
					entt::handle connectedEntity = distribution->GetOutputConnectedEntity(outputIndex);

					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.UpdateEntityDistributionConnection(handle, outputIndex8, connectedEntity, inputIndex8);
					});
				});
			}


			if (PlanetComponent* planetComponent = m_registry.try_get<PlanetComponent>(entity))
			{
				// FIXME: Deduplicate code (maybe by merging components)
				entityData.onChunkAdded.Connect(planetComponent->planet->OnChunkAdded, [this, entity](ChunkContainer* /*container*/, Chunk* chunk)
				{
					entt::handle handle(m_registry, entity);
					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.CreateChunk(handle, *chunk);
					});
				});

				entityData.onChunkRemove.Connect(planetComponent->planet->OnChunkRemove, [this, entity](ChunkContainer* /*container*/, Chunk* chunk)
				{
					entt::handle handle(m_registry, entity);
					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.DestroyChunk(handle, *chunk);
					});
				});
			}

			if (ShipComponent* shipComponent = m_registry.try_get<ShipComponent>(entity))
			{
				// FIXME: Deduplicate code (maybe by merging components)
				entityData.onChunkAdded.Connect(shipComponent->ship->OnChunkAdded, [this, entity](ChunkContainer* /*container*/, Chunk* chunk)
				{
					entt::handle handle(m_registry, entity);
					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.CreateChunk(handle, *chunk);
					});
				});

				entityData.onChunkRemove.Connect(shipComponent->ship->OnChunkRemove, [this, entity](ChunkContainer* /*container*/, Chunk* chunk)
				{
					entt::handle handle(m_registry, entity);
					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.DestroyChunk(handle, *chunk);
					});
				});
			}

			auto& entityNetwork = m_registry.get<NetworkedComponent>(entity);
			if (!entityNetwork.ShouldSignalCreation())
				continue;

			SessionVisibilityHandler::CreateEntityData createData = BuildCreateEntityData(entity);
			ForEachVisibility([&](SessionVisibilityHandler& visibility)
			{
				CreateEntity(visibility, entt::handle(m_registry, entity), createData);
			});
		}
		m_pendingCreatedEntities.clear();

		if (!m_pendingPlayers.empty())
		{
			// Send all entities to newly connected players
			for (auto&& [entity, entityData] : m_networkedEntities.each())
			{
				entt::handle entityHandle(m_registry, entity);

				SessionVisibilityHandler::CreateEntityData createData = BuildCreateEntityData(entity);
				for (ServerPlayer* player : m_pendingPlayers)
					CreateEntity(player->GetVisibilityHandler(), entityHandle, createData);
			}

			for (ServerPlayer* player : m_pendingPlayers)
				m_players.RegisterPlayer(player);

			m_pendingPlayers.clear();
		}
	}

	SessionVisibilityHandler::CreateEntityData NetworkedEntitiesSystem::BuildCreateEntityData(entt::entity entity) const
	{
		bool isMoving = m_registry.any_of<Nz::PhysCharacter3DComponent, Nz::RigidBody3DComponent>(entity);

		auto& entityNode = m_registry.get<Nz::NodeComponent>(entity);

		auto& entityNet = m_registry.get<NetworkedComponent>(entity);

		auto* entityInstance = m_registry.try_get<ClassInstanceComponent>(entity);

		SessionVisibilityHandler::CreateEntityData createData;
		createData.entityClass = (entityInstance) ? entityInstance->GetClass() : nullptr;
		createData.environment = &m_environment;
		createData.initialPosition = entityNode.GetPosition();
		createData.initialRotation = entityNode.GetRotation();
		createData.isMoving = isMoving;

		if (entityInstance)
		{
			createData.entityProperties.reserve(createData.entityClass->GetPropertyCount());
			for (std::size_t i = 0; i < createData.entityClass->GetPropertyCount(); ++i)
			{
				if (entityInstance->GetClass()->GetProperty(i).isNetworked)
					createData.entityProperties.emplace_back(entityInstance->GetProperty(i));
			}
		}

		if (auto* playerControlled = m_registry.try_get<ServerPlayerControlledComponent>(entity))
		{
			if (ServerPlayer* controllingPlayer = playerControlled->GetPlayer())
			{
				auto& data = createData.playerControlledData.emplace();
				data.controllingPlayerId = controllingPlayer->GetPlayerIndex();
			}
		}

		return createData;
	}

	void NetworkedEntitiesSystem::CreateEntity(SessionVisibilityHandler& visibility, entt::handle entity, const SessionVisibilityHandler::CreateEntityData& createData) const
	{
		entt::handle handle(m_registry, entity);
		visibility.CreateEntity(handle, createData);

		// Connections are sent afterward to handle entity dependency issues
		if (DistributionComponent* entityDistribution = m_registry.try_get<DistributionComponent>(entity))
		{
			for (std::size_t outputIndex = 0; outputIndex < entityDistribution->GetOutputCount(); ++outputIndex)
			{
				Nz::UInt8 outputIndex8 = Nz::SafeCaster(outputIndex);
				Nz::UInt8 inputIndex8 = Nz::SafeCaster(entityDistribution->GetOutputConnectedPort(outputIndex));
				entt::handle connectedEntity = entityDistribution->GetOutputConnectedEntity(outputIndex);

				if (connectedEntity)
					visibility.UpdateEntityDistributionConnection(handle, outputIndex8, connectedEntity, inputIndex8);
			}
		}

		if (PlanetComponent* planetComponent = handle.try_get<PlanetComponent>())
		{
			planetComponent->planet->ForEachChunk([&](const ChunkIndices& /*chunkIndices*/, Chunk& chunk)
			{
				visibility.CreateChunk(handle, chunk);
			});
		}

		if (ShipComponent* shipComponent = handle.try_get<ShipComponent>())
		{
			shipComponent->ship->ForEachChunk([&](const ChunkIndices& /*chunkIndices*/, Chunk& chunk)
			{
				visibility.CreateChunk(handle, chunk);
			});
		}
	}
}
