// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_CHUNKCONTAINERCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_CHUNKCONTAINERCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <memory>

namespace tsom
{
	struct ChunkContainerComponent
	{
		std::shared_ptr<ChunkContainer> chunkContainer;
		std::array<std::unique_ptr<ChunkEntities>, Constants::MaxChunkLayerCount> chunkEntities;
	};
}

#include <CommonLib/Components/ChunkContainerComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_CHUNKCONTAINERCOMPONENT_HPP
