// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	void EnvironmentSwitchSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<Nz::NodeComponent, EnvironmentEnterTriggerComponent>();
		auto switchView = m_registry.view<Nz::NodeComponent, ServerEnvironmentSwitchComponent>();

		for (auto&& [triggerEntity, triggerNode, enterTrigger] : view.each())
		{
			EnvironmentTransform transform(triggerNode.GetPosition(), triggerNode.GetRotation());
			transform = -transform;

			for (auto&& [entity, entityNode, envSwitch] : switchView.each())
			{
				if (entity == triggerEntity)
					continue;

				Nz::Vector3f entityPosition = entityNode.GetPosition();

				Nz::Vector3f localPlayerPos = triggerNode.ToLocalPosition(entityPosition);
				// Use AABB as a cheap test
				if (enterTrigger.aabb.Contains(localPlayerPos))
				{
					if (enterTrigger.entryTrigger)
					{
						localPlayerPos -= enterTrigger.entryTrigger->GetCenterOfMass(); //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
						if (enterTrigger.entryTrigger->CollisionQuery(localPlayerPos))
							envSwitch.handleEnvironmentSwitch(entt::handle(m_registry, entity), enterTrigger.targetEnvironment, transform);
					}
					else
						envSwitch.handleEnvironmentSwitch(entt::handle(m_registry, entity), enterTrigger.targetEnvironment, transform);
				}
			}

			m_ownerEnvironment->ForEachPlayer([&](ServerPlayer& player)
			{
				if (player.GetControlledEntityEnvironment() != m_ownerEnvironment)
					return;

				entt::handle playerEntity = player.GetControlledEntity();
				if (!playerEntity)
					return;

				Nz::Vector3f playerPosition = playerEntity.get<Nz::NodeComponent>().GetPosition();

				Nz::Vector3f localPlayerPos = triggerNode.ToLocalPosition(playerPosition);
				// Use AABB as a cheap test
				if (enterTrigger.aabb.Contains(localPlayerPos))
				{
					if (enterTrigger.entryTrigger)
					{
						localPlayerPos -= enterTrigger.entryTrigger->GetCenterOfMass(); //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
						if (enterTrigger.entryTrigger->CollisionQuery(localPlayerPos))
							player.MoveEntityToEnvironment(enterTrigger.targetEnvironment, transform, Nz::Vector3f::Zero(), enterTrigger.updateRoot);
					}
					else
						player.MoveEntityToEnvironment(enterTrigger.targetEnvironment, transform, Nz::Vector3f::Zero(), enterTrigger.updateRoot);
				}
			});
		}
	}
}
