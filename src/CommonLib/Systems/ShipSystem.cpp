// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/ShipSystem.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void ShipSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<ShipComponent>();
		for (auto&& [entity, ship] : view.each())
		{
			NazaraUnused(entity);
			ship.chunkEntities->Update();
		}
	}
}
