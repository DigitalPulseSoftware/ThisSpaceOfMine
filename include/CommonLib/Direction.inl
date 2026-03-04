// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Assert.hpp>

namespace tsom
{
	constexpr Direction DirectionFromNormal(const Nz::Vector3f& outsideNormal)
	{
		Direction closestDir;
		float closestDirDot = -1.f;
		for (auto&& [direction, normal] : s_dirNormals.iter_kv())
		{
			if (float dot = outsideNormal.DotProduct(normal); dot >= closestDirDot)
			{
				closestDir = direction;
				closestDirDot = dot;
			}
		}

		return closestDir;
	}

	constexpr NeighborChunk ToNeighborChunk(const Nz::Vector3i32& chunkIndices)
	{
		NazaraAssert(chunkIndices.x >= -1 && chunkIndices.x <= 1);
		NazaraAssert(chunkIndices.y >= -1 && chunkIndices.y <= 1);
		NazaraAssert(chunkIndices.z >= -1 && chunkIndices.z <= 1);

		return static_cast<NeighborChunk>((chunkIndices.x + 1) * 9 + (chunkIndices.y + 1) * 3 + (chunkIndices.z + 1));
	}
}
