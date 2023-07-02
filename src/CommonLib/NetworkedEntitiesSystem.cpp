// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkedEntitiesSystem.hpp>
#include <CommonLib/ServerWorld.hpp>
#include <CommonLib/Components/NetworkedComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltCharacterComponent.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltRigidBody3DComponent.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>

namespace tsom
{
	NetworkedEntitiesSystem::NetworkedEntitiesSystem(ServerWorld& world, entt::registry& registry) :
	m_networkedConstructObserver(registry, entt::collector.group<Nz::NodeComponent, NetworkedComponent>(entt::exclude<Nz::DisabledComponent>)),
	m_registry(registry),
	m_world(world)
	{
		m_networkedDestroyConnection = m_registry.on_construct<Nz::DisabledComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
		m_networkedDestroyConnection = m_registry.on_destroy<NetworkedComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
		m_nodeDestroyConnection = m_registry.on_destroy<Nz::NodeComponent>().connect<&NetworkedEntitiesSystem::OnNetworkedDestroy>(this);
	}

	void NetworkedEntitiesSystem::ForEachVisibility(const Nz::FunctionRef<void(SessionVisibilityHandler& visibility)>& functor)
	{
		m_world.ForEachPlayer([&](ServerPlayer& player)
		{
			functor(player.GetVisibilityHandler());
		});
	}

	void NetworkedEntitiesSystem::Update(Nz::Time elapsedTime)
	{
		m_networkedConstructObserver.each([&](entt::entity entity)
		{
			if (m_registry.try_get<Nz::JoltCharacterComponent>(entity) || m_registry.try_get<Nz::JoltRigidBody3DComponent>(entity))
				m_movingEntities.insert(entity);


		});


	}

	void NetworkedEntitiesSystem::OnNetworkedDestroy([[maybe_unused]] entt::registry& registry, entt::entity entity)
	{
		assert(&m_registry == &registry);

		m_movingEntities.erase(entity);
	}
}
