// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/EnvironmentProxySystem.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

namespace tsom
{
	EnvironmentProxySystem::EnvironmentProxySystem(entt::registry& registry) :
	m_observer(registry, entt::collector.group<Nz::NodeComponent, EnvironmentProxyComponent>(entt::exclude<Nz::DisabledComponent>)),
	m_registry(registry)
	{
	}

	void EnvironmentProxySystem::AddEnvironmentRecursively(ServerPlayer* player)
	{
		auto& visibilityHandler = player->GetVisibilityHandler();

		auto view = m_registry.view<Nz::NodeComponent, EnvironmentProxyComponent>(entt::exclude<Nz::DisabledComponent>);
		for (entt::entity entity : view)
		{
			auto& envProxy = view.get<EnvironmentProxyComponent>(entity);
			if (player->IsInEnvironment(envProxy.targetEnvironment))
				continue; // break recursion

			player->AddToEnvironment(envProxy.targetEnvironment, entt::handle(m_registry, entity));

			auto& envProxySystem = envProxy.targetEnvironment->GetWorld().GetSystem<EnvironmentProxySystem>();
			envProxySystem.AddEnvironmentRecursively(player);
		}
	}

	void EnvironmentProxySystem::Update(Nz::Time /*elapsedTime*/)
	{
		ServerEnvironment* environment = ServerEnvironment::GetEnvironment(m_registry);

		m_observer.each([&](entt::entity entity)
		{
			auto& envProxy = m_registry.get<EnvironmentProxyComponent>(entity);
			environment->ForEachPlayer([&](ServerPlayer& player)
			{
				if (player.IsInEnvironment(envProxy.targetEnvironment))
					return;

				player.AddToEnvironment(envProxy.targetEnvironment, entt::handle(m_registry, entity));

				auto& envProxySystem = envProxy.targetEnvironment->GetWorld().GetSystem<EnvironmentProxySystem>();
				envProxySystem.AddEnvironmentRecursively(&player);
			});
		});
	}
}
