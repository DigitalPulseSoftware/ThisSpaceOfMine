// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_CHUNKCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_CHUNKCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <NazaraUtils/MovablePtr.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	struct ChunkComponent
	{
		Nz::MovablePtr<Chunk> chunk;
		entt::handle parentEntity;
	};
}

#include <CommonLib/Components/ChunkComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_CHUNKCOMPONENT_HPP
