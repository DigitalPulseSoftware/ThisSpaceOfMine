// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Entities/ServerChunkClassLibrary.hpp>
#include <CommonLib/Asteroid.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	std::unique_ptr<ChunkEntities> ServerChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer, std::size_t layerIndex, bool isStatic)
	{
		return std::make_unique<ChunkEntities>(m_app, world, chunkContainer, m_blockLibrary, layerIndex, isStatic);
	}
}
