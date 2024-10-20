// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/ShipSystem.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void ShipSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<ShipComponent>(entt::exclude<Nz::DisabledComponent>);
		for (entt::entity entity : view)
		{
			ShipComponent& shipComponent = view.get<ShipComponent>(entity);
			shipComponent.shipEntities->Update();
		}
	}
}
