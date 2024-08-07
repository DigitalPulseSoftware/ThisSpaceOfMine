// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	void EnvironmentSwitchSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<Nz::NodeComponent, EnvironmentEnterTriggerComponent>();

		for (auto&& [entity, triggerNode, enterTrigger] : view.each())
		{
			if (!enterTrigger.entryTrigger)
				continue;

			m_ownerEnvironment->ForEachPlayer([&](ServerPlayer& player)
			{
				entt::handle playerEntity = player.GetControlledEntity();
				if (!playerEntity)
					return;

				Nz::Vector3f playerPosition = playerEntity.get<Nz::NodeComponent>().GetPosition();

				if (player.GetControlledEntityEnvironment() == m_ownerEnvironment)
				{
					Nz::Vector3f localPlayerPos = triggerNode.ToLocalPosition(playerPosition);
					// Use AABB as a cheap test
					if (enterTrigger.aabb.Contains(localPlayerPos) && enterTrigger.entryTrigger->CollisionQuery(localPlayerPos))
						player.MoveEntityToEnvironment(enterTrigger.targetEnvironment);
				}
			});
		}
	}
}
