// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_COMPLEXCOLLISIONCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_COMPLEXCOLLISIONCOMPONENT_HPP

#include <memory>
#include <vector>

namespace Nz
{
	class Collider3D;
	class CompoundCollider3D;
}

namespace tsom
{
	struct ComplexCollisionComponent
	{
		std::shared_ptr<Nz::Collider3D> complexCollisionCollider;
	};

	struct ComplexChunkCollisionComponent
	{
		struct ChunkEntry
		{
			entt::handle chunkEntity;
			std::shared_ptr<Nz::Collider3D> chunkCollider;
			Nz::Vector3f chunkOffset;
		};

		std::vector<ChunkEntry> chunkEntries;
	};
}

#endif // TSOM_COMMONLIB_COMPONENTS_COMPLEXCOLLISIONCOMPONENT_HPP
