// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void ChunkEntities::UpdateChunkEntity(const ChunkIndices& chunkIndices, DirectionMask neighborMask)
	{
		assert(m_chunkEntities.contains(chunkIndices));
		const Chunk* chunk = m_chunkContainer.GetChunk(chunkIndices);
		assert(chunk);

		ProcessChunkUpdate(chunk, neighborMask);
	}
}
