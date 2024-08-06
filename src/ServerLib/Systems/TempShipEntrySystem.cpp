// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/TempShipEntrySystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/TempShipEntryComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	void TempShipEntrySystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<Nz::NodeComponent, TempShipEntryComponent>();

		for (auto&& [entity, node, shipEntry] : view.each())
		{
			if (!shipEntry.entryTrigger)
				continue;

			m_ownerEnvironment->ForEachPlayer([&](ServerPlayer& player)
			{
				entt::handle entity = player.GetControlledEntity();
				if (!entity)
					return;

				Nz::Vector3f playerPosition = entity.get<Nz::NodeComponent>().GetPosition();

				if (player.GetControlledEntityEnvironment() == m_ownerEnvironment)
				{
					Nz::Vector3f localPlayerPos = node.ToLocalPosition(playerPosition);
					// Use AABB as a cheap test
					if (shipEntry.aabb.Contains(localPlayerPos) && shipEntry.entryTrigger->CollisionQuery(localPlayerPos))
						player.MoveEntityToEnvironment(shipEntry.shipEnv);
				}
			});
		}
	}
}
