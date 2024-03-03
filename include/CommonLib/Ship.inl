// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::Vector3f Ship::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline Chunk* Ship::GetChunk(std::size_t chunkIndex)
	{
		return m_chunks[chunkIndex].chunk.get();
	}

	inline const Chunk* Ship::GetChunk(std::size_t chunkIndex) const
	{
		return m_chunks[chunkIndex].chunk.get();
	}

	inline Chunk& Ship::GetChunk(const Nz::Vector3ui& indices)
	{
		return *m_chunks[GetChunkIndex(indices)].chunk;
	}

	inline const Chunk& Ship::GetChunk(const Nz::Vector3ui& indices) const
	{
		return *m_chunks[GetChunkIndex(indices)].chunk;
	}

	inline Nz::Vector3ui Ship::GetChunkIndices(std::size_t chunkIndex) const
	{
		Nz::Vector3ui indices;
		indices.x = chunkIndex % ChunkSize;
		indices.y = (chunkIndex / ChunkSize) % ChunkSize;
		indices.z = chunkIndex / (ChunkSize * ChunkSize);

		return indices;
	}

	inline std::size_t Ship::GetChunkCount() const
	{
		return m_chunks.size();
	}
}
