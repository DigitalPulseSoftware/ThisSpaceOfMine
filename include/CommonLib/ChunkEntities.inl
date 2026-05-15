// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline const ChunkContainer& ChunkEntities::GetChunkContainer() const
	{
		return m_chunkContainer;
	}

	inline std::size_t ChunkEntities::GetLayerIndex() const
	{
		return m_layerIndex;
	}

	inline void ChunkEntities::UpdateChunkEntity(const ChunkIndices& chunkIndices, NeighborChunkMask neighborMask)
	{
		assert(m_chunkEntities.contains(chunkIndices));
		const Chunk* chunk = m_chunkContainer.GetChunk(chunkIndices);
		assert(chunk);

		ProcessChunkUpdate(*chunk, neighborMask);
	}
}
