// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/NetworkedEntitiesSystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/PhysCharacter3DComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	NetworkedEntitiesSystem::NetworkedEntitiesSystem(entt::registry& registry, ServerInstance& instance) :
	m_networkedConstructObserver(registry, entt::collector.group<Nz::NodeComponent, NetworkedComponent>(entt::exclude<Nz::DisabledComponent>)),
	m_registry(registry),
	m_instance(instance)
	{
		m_disabledConstructConnection = m_registry.on_construct<Nz::DisabledComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
		m_networkedDestroyConnection = m_registry.on_destroy<NetworkedComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
		m_nodeDestroyConnection = m_registry.on_destroy<Nz::NodeComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
	}

	void NetworkedEntitiesSystem::CreateAllEntities(SessionVisibilityHandler& visibility) const
	{
		auto networkedView = m_registry.view<Nz::NodeComponent, NetworkedComponent>(entt::exclude<Nz::DisabledComponent>);
		for (entt::entity entity : networkedView)
			visibility.CreateEntity(entt::handle(m_registry, entity), BuildCreateEntityData(entity));
	}

	void NetworkedEntitiesSystem::ForEachVisibility(const Nz::FunctionRef<void(SessionVisibilityHandler& visibility)>& functor)
	{
		m_instance.ForEachPlayer([&](ServerPlayer& player)
		{
			functor(player.GetVisibilityHandler());
		});
	}

	void NetworkedEntitiesSystem::Update(Nz::Time elapsedTime)
	{
		m_networkedConstructObserver.each([&](entt::entity entity)
		{
			SessionVisibilityHandler::CreateEntityData createData = BuildCreateEntityData(entity);
			if (createData.isMoving)
				m_movingEntities.insert(entity);

			ForEachVisibility([&](SessionVisibilityHandler& visibility)
			{
				visibility.CreateEntity(entt::handle(m_registry, entity), createData);
			});
		});
	}

	SessionVisibilityHandler::CreateEntityData NetworkedEntitiesSystem::BuildCreateEntityData(entt::entity entity) const
	{
		bool isMoving = m_registry.try_get<Nz::PhysCharacter3DComponent>(entity) || m_registry.try_get<Nz::RigidBody3DComponent>(entity);

		auto& entityNode = m_registry.get<Nz::NodeComponent>(entity);

		SessionVisibilityHandler::CreateEntityData createData;
		createData.initialPosition = entityNode.GetPosition();
		createData.initialRotation = entityNode.GetRotation();
		createData.isMoving = isMoving;

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

	void NetworkedEntitiesSystem::OnNetworkedDestroy([[maybe_unused]] entt::registry& registry, entt::entity entity)
	{
		assert(&m_registry == &registry);

		m_movingEntities.erase(entity);

		ForEachVisibility([&](SessionVisibilityHandler& visibility)
		{
			visibility.DestroyEntity(entt::handle(m_registry, entity));
		});
	}
}
