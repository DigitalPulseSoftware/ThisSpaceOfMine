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

		for (entt::entity entity : view)
		{
			auto& enterTrigger = view.get<EnvironmentEnterTriggerComponent>(entity);
			if (!enterTrigger.entryTrigger)
				continue;

			auto& triggerNode = view.get<Nz::NodeComponent>(entity);
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
					localPlayerPos -= enterTrigger.entryTrigger->GetCenterOfMass(); //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
					if (enterTrigger.entryTrigger->CollisionQuery(localPlayerPos))
						player.MoveEntityToEnvironment(enterTrigger.targetEnvironment);
				}
			});
		}
	}
}
