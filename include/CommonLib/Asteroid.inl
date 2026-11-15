// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::Vector3f Asteroid::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline Chunk* Asteroid::GetChunk(const ChunkIndices& chunkIndices)
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline const Chunk* Asteroid::GetChunk(const ChunkIndices& chunkIndices) const
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline std::size_t Asteroid::GetChunkCount() const
	{
		return m_chunks.size();
	}

	inline float Asteroid::GetRadius() const
	{
		return m_radius;
	}
}
