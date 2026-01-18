// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
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
	m_environment(environment)
	{
		m_networkedEntities.OnEntityAdded.Connect([&](entt::entity entity)
		{
			// Handle entities in Update to let all components initialize themselves
			m_pendingEntities.emplace(entity);
		});

		m_networkedEntities.OnEntityRemove.Connect([&](entt::entity entity)
		{
			if (!m_pendingEntities.remove(entity))
			{
				ForEachVisibility([&](SessionVisibilityHandler& visibility)
				{
					visibility.DestroyEntity(entt::handle(m_registry, entity));
				});
			}
		});
	}

	void NetworkedEntitiesSystem::CreateAllEntities(SessionVisibilityHandler& visibility) const
	{
		for (auto&& [entity, entityData] : m_networkedEntities.each())
			CreateEntity(visibility, entt::handle(m_registry, entity), BuildCreateEntityData(entity));
	}

	void NetworkedEntitiesSystem::ForEachVisibility(const Nz::FunctionRef<void(SessionVisibilityHandler& visibility)>& functor)
	{
		m_environment.ForEachPlayer([&](ServerPlayer& player)
		{
			functor(player.GetVisibilityHandler());
		});
	}

	void NetworkedEntitiesSystem::ForgetEntity(entt::entity entity)
	{
		m_networkedEntities.Remove(entity);
	}

	void NetworkedEntitiesSystem::Update(Nz::Time /*elapsedTime*/)
	{
		for (entt::entity entity : m_pendingEntities)
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

			auto& entityNetwork = m_registry.get<NetworkedComponent>(entity);
			if (!entityNetwork.ShouldSignalCreation())
				return;

			SessionVisibilityHandler::CreateEntityData createData = BuildCreateEntityData(entity);
			ForEachVisibility([&](SessionVisibilityHandler& visibility)
			{
				CreateEntity(visibility, entt::handle(m_registry, entity), createData);
			});
		}
		m_pendingEntities.clear();
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
