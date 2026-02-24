// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cassert>

namespace tsom
{
	inline std::size_t SurfaceNetsChunk::GetNeighborIndex(const ChunkIndices& chunkIndices)
	{
		assert(chunkIndices.x >= -1 && chunkIndices.x <= -1);
		assert(chunkIndices.y >= -1 && chunkIndices.y <= -1);
		assert(chunkIndices.z >= -1 && chunkIndices.z <= -1);

		return (chunkIndices.x + 1) * 9 + (chunkIndices.y + 1) * 3 + (chunkIndices.z + 1);
	}
}
