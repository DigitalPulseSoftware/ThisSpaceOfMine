// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/PlanetSystem.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void PlanetSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<PlanetComponent>(entt::exclude<Nz::DisabledComponent>);
		for (entt::entity entity : view)
		{
			PlanetComponent& planetComponent = view.get<PlanetComponent>(entity);
			for (const auto& chunkEntitiesPtr : planetComponent.planetEntities)
			{
				if (chunkEntitiesPtr)
					chunkEntitiesPtr->Update();
			}
		}
	}
}
