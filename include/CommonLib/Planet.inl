// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::Vector3f Planet::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline Chunk* Planet::GetChunk(const ChunkIndices& chunkIndices)
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline const Chunk* Planet::GetChunk(const ChunkIndices& chunkIndices) const
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline std::size_t Planet::GetChunkCount() const
	{
		return m_chunks.size();
	}

	inline float Planet::GetCornerRadius() const
	{
		return m_cornerRadius;
	}

	inline float Planet::GetGravityFactor(const Nz::Vector3f& position) const
	{
		if (position.SquaredDistance(GetCenter()) < Nz::IntegralPow(10.f, 2))
			return 0.f;

		return m_gravityFactor;
	}

	inline void Planet::UpdateCornerRadius(float cornerRadius)
	{
		m_cornerRadius = cornerRadius;
	}
}
