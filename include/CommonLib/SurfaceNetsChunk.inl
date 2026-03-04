// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline SurfaceNetsChunk::SurfaceNetsChunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float blockSize) :
	Chunk(blockLibrary, owner, indices, size, blockSize)
	{
		SetPerFaceCollision();
	}
}
