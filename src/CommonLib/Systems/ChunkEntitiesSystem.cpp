// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/ChunkEntitiesSystem.hpp>
#include <CommonLib/Components/ChunkContainerComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void ChunkEntitiesSystem::Update(Nz::Time /*elapsedTime*/)
	{
		auto chunkContainerView = m_registry.view<ChunkContainerComponent>(entt::exclude<Nz::DisabledComponent>);
		for (entt::entity entity : chunkContainerView)
		{
			ChunkContainerComponent& chunkContainerComponent = chunkContainerView.get<ChunkContainerComponent>(entity);
			for (const auto& chunkEntitiesPtr : chunkContainerComponent.chunkEntities)
			{
				if (chunkEntitiesPtr)
					chunkEntitiesPtr->Update();
			}
		}
	}
}
