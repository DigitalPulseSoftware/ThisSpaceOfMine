// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <CommonLib/Planet.hpp>
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
		for (entt::entity entity : m_networkedEntities)
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
		m_networkedEntities.erase(entity);
		m_movingEntities.erase(entity);
	}

	void NetworkedEntitiesSystem::Update(Nz::Time elapsedTime)
	{
		m_networkedConstructObserver.each([&](entt::entity entity)
		{
			assert(!m_networkedEntities.contains(entity));
			m_networkedEntities.insert(entity);

			auto& entityNetwork = m_registry.get<NetworkedComponent>(entity);
			if (!entityNetwork.ShouldSignalCreation())
				return;

			SessionVisibilityHandler::CreateEntityData createData = BuildCreateEntityData(entity);
			if (createData.isMoving)
				m_movingEntities.insert(entity);

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

		SessionVisibilityHandler::CreateEntityData createData;
		createData.environment = &m_environment;
		createData.initialPosition = entityNode.GetPosition();
		createData.initialRotation = entityNode.GetRotation();
		createData.isMoving = isMoving;

		if (auto* planetComp = m_registry.try_get<PlanetComponent>(entity))
		{
			auto& data = createData.planetData.emplace();
			data.cellSize = planetComp->GetTileSize();
			data.cornerRadius = planetComp->GetCornerRadius();
			data.gravity = planetComp->GetGravity();
		}

		if (auto* playerControlled = m_registry.try_get<ServerPlayerControlledComponent>(entity))
		{
			if (ServerPlayer* controllingPlayer = playerControlled->GetPlayer())
			{
				auto& data = createData.playerControlledData.emplace();
				data.controllingPlayerId = controllingPlayer->GetPlayerIndex();
			}
		}

		if (auto* shipComp = m_registry.try_get<ShipComponent>(entity))
		{
			auto& data = createData.shipData.emplace();
			data.cellSize = shipComp->GetTileSize();
		}

		return createData;
	}

	void NetworkedEntitiesSystem::CreateEntity(SessionVisibilityHandler& visibility, entt::handle entity, const SessionVisibilityHandler::CreateEntityData& createData) const
	{
		entt::handle handle(m_registry, entity);
		visibility.CreateEntity(handle, createData);

		if (PlanetComponent* planet = handle.try_get<PlanetComponent>())
		{
			planet->ForEachChunk([&](const ChunkIndices& /*chunkIndices*/, Chunk& chunk)
			{
				visibility.CreateChunk(handle, chunk);
			});
		}

		if (ShipComponent* ship = handle.try_get<ShipComponent>())
		{
			ship->ForEachChunk([&](const ChunkIndices& /*chunkIndices*/, Chunk& chunk)
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
		m_movingEntities.erase(entity);

		ForEachVisibility([&](SessionVisibilityHandler& visibility)
		{
			visibility.DestroyEntity(entt::handle(m_registry, entity));
		});
	}
}
