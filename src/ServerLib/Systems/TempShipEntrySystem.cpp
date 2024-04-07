// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/TempShipEntrySystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/TempShipEntryComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

namespace tsom
{
	void TempShipEntrySystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<Nz::NodeComponent, TempShipEntryComponent>();

		ServerPlanetEnvironment& planetEnvironment = m_serverInstance.GetPlanetEnvironment();
		m_serverInstance.ForEachPlayer([&](ServerPlayer& player)
		{
			entt::handle entity = player.GetControlledEntity();
			if (!entity)
				return;

			Nz::Vector3f playerPosition = entity.get<Nz::NodeComponent>().GetPosition();

			if (player.GetControlledEntityEnvironment() == &planetEnvironment)
			{
				for (auto&& [entity, node, shipEntry] : view.each())
				{
					Nz::Vector3f localPlayerPos = node.ToLocalPosition(playerPosition);
					if (shipEntry.aabb.Contains(localPlayerPos))
					{
						player.MoveEntityToEnvironment(shipEntry.shipEnv);
						break;
					}
				}
			}
			else
			{
				for (auto&& [entity, node, shipEntry] : view.each())
				{
					if (player.GetControlledEntityEnvironment() != shipEntry.shipEnv)
						continue;

					if (!shipEntry.aabb.Contains(playerPosition))
					{
						player.MoveEntityToEnvironment(&planetEnvironment);
						break;
					}
				}
			}
		});
	}
}
