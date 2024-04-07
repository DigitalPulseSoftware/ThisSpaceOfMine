// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/EnvironmentProxySystem.hpp>
#include <CommonLib/EnvironmentTransform.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/SessionVisibilityHandler.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <entt/entt.hpp>

#include <ServerLib/ServerShipEnvironment.hpp>

namespace tsom
{
	void EnvironmentProxySystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<Nz::NodeComponent, EnvironmentProxyComponent>();
		for (entt::entity entity : view)
		{
			auto& nodeComponent = view.get<Nz::NodeComponent>(entity);
			auto& proxyComponent = view.get<EnvironmentProxyComponent>(entity);

			EnvironmentTransform relativeTransform(nodeComponent.GetPosition(), nodeComponent.GetRotation());
			proxyComponent.fromEnv->UpdateConnectedTransform(*proxyComponent.toEnv, relativeTransform);
			proxyComponent.toEnv->UpdateConnectedTransform(*proxyComponent.fromEnv, -relativeTransform);

			proxyComponent.fromEnv->ForEachPlayer([&](ServerPlayer& player)
			{
				player.GetVisibilityHandler().MoveEnvironment(*proxyComponent.toEnv, relativeTransform);
			});
		}
	}
}
