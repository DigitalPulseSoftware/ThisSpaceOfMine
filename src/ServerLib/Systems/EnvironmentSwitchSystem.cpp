// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	void EnvironmentSwitchSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<Nz::NodeComponent, EnvironmentEnterTriggerComponent>();
		auto switchView = m_registry.view<Nz::NodeComponent, ClassInstanceComponent, ServerEnvironmentSwitchComponent>();

		ServerEnvironment* previousEnvironment = ServerEnvironment::GetEnvironment(m_registry);

		for (auto&& [triggerEntity, triggerNode, enterTrigger] : view.each())
		{
			if (!enterTrigger.enabled)
				continue;

			EnvironmentTransform transform(triggerNode.GetPosition(), triggerNode.GetRotation());
			transform = -transform;

			for (auto&& [entity, entityNode, entityInstance, envSwitch] : switchView.each())
			{
				if (entity == triggerEntity)
					continue;

				Nz::Vector3f entityPosition = entityNode.GetPosition();

				Nz::Vector3f localPlayerPos = triggerNode.ToLocalPosition(entityPosition);
				// Use AABB as a cheap test
				if NAZARA_LIKELY(!enterTrigger.aabb.Contains(localPlayerPos))
					continue;

				if (enterTrigger.entryTrigger)
				{
					if (!enterTrigger.entryTrigger->CollisionQuery(localPlayerPos - enterTrigger.entryTrigger->GetCenterOfMass()))  //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
						continue;
				}

				entt::handle oldEntity(m_registry, entity);
				ServerEnvironment* newEnvironment = enterTrigger.targetEnvironment;

				envSwitch.Switch(oldEntity, previousEnvironment, newEnvironment, transform);
			}
		}
	}
}
