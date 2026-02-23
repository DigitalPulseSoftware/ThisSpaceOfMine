// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/EnvironmentSwitchSystem.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	EnvironmentSwitchSystem::EnvironmentSwitchSystem(entt::registry& registry) :
	m_registry(registry)
	{
		m_ownerEnvironment = m_registry.ctx().get<ServerEnvironment*>();
	}

	void EnvironmentSwitchSystem::Update(Nz::Time elapsedTime)
	{
		auto triggerView = m_registry.view<Nz::NodeComponent, EnvironmentEnterTriggerComponent>(entt::exclude<Nz::DisabledComponent>);
		auto entityView = m_registry.view<Nz::NodeComponent, ClassInstanceComponent, ServerEnvironmentSwitchComponent>(entt::exclude<Nz::DisabledComponent>);

		ServerEnvironment* previousEnvironment = ServerEnvironment::GetEnvironment(m_registry);

		for (auto&& [triggerEntity, triggerNode, enterTrigger] : triggerView.each())
		{
			if (!enterTrigger.enabled)
				continue;

			for (auto&& [entity, entityNode, entityInstance, envSwitch] : entityView.each())
			{
				if (entity == triggerEntity)
					continue;

				Nz::Vector3f localPos = triggerNode.ToLocalPosition(entityNode.GetPosition());

				// Use AABB as a cheap test
				if NAZARA_LIKELY(!enterTrigger.aabb.Contains(localPos))
					continue;

				if (enterTrigger.entryTrigger)
				{
					if (!enterTrigger.entryTrigger->CollisionQuery(localPos - enterTrigger.entryTrigger->GetCenterOfMass())) //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
						continue;
				}

				entt::handle oldEntity(m_registry, entity);
				ServerEnvironment* newEnvironment = enterTrigger.targetEnvironment;

				EnvironmentTransform transform(triggerNode.GetPosition(), triggerNode.GetRotation());
				transform = -transform;

				envSwitch.Switch(oldEntity, previousEnvironment, newEnvironment, transform);
				break;
			}
		}
	}
}
