// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
	m_networkedConstructObserver(registry, entt::collector.group<Nz::NodeComponent, NetworkedComponent>(entt::exclude<Nz::DisabledComponent>)),
	m_registry(registry),
	m_environment(environment)
	{
		m_disabledConstructConnection = m_registry.on_construct<Nz::DisabledComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
		m_networkedDestroyConnection = m_registry.on_destroy<NetworkedComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
		m_nodeDestroyConnection = m_registry.on_destroy<Nz::NodeComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
	}

	void NetworkedEntitiesSystem::CreateAllEntities(SessionVisibilityHandler& visibility) const
	{
		for (auto it = m_networkedEntities.begin(); it != m_networkedEntities.end(); ++it)
			CreateEntity(visibility, entt::handle(m_registry, it.key()), BuildCreateEntityData(it.key()));
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
		m_networkedEntities.erase(entity);
	}

	void NetworkedEntitiesSystem::Update(Nz::Time elapsedTime)
	{
		m_networkedConstructObserver.each([&](entt::entity entity)
		{
			assert(!m_networkedEntities.contains(entity));
			EntityData& entityData = m_networkedEntities[entity];

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

				entityData.onPropertyUpdate.Connect(entityInstance->OnPropertyUpdate, [this, entity](ClassInstanceComponent* emitter, Nz::UInt32 propertyIndex, const EntityProperty& /*newValue*/)
				{
					if (!emitter->GetClass()->GetProperty(propertyIndex).isNetworked)
						return;

					entt::handle handle(m_registry, entity);
					ForEachVisibility([&](SessionVisibilityHandler& visibility)
					{
						visibility.UpdateEntityProperty(handle, propertyIndex);
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
		});
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

	void NetworkedEntitiesSystem::OnNetworkedDestroy([[maybe_unused]] entt::registry& registry, entt::entity entity)
	{
		assert(&m_registry == &registry);

		if (!m_networkedEntities.contains(entity))
			return;

		m_networkedEntities.erase(entity);

		ForEachVisibility([&](SessionVisibilityHandler& visibility)
		{
			visibility.DestroyEntity(entt::handle(m_registry, entity));
		});
	}
}
