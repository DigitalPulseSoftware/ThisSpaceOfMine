// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline Nz::Vector3f Planet::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline Chunk* Planet::GetChunk(std::size_t chunkIndex)
	{
		return m_chunks[chunkIndex].chunk.get();
	}

	inline const Chunk* Planet::GetChunk(std::size_t chunkIndex) const
	{
		return m_chunks[chunkIndex].chunk.get();
	}

	inline Chunk& Planet::GetChunk(const Nz::Vector3ui& indices)
	{
		return *m_chunks[GetChunkIndex(indices)].chunk;
	}

	inline const Chunk& Planet::GetChunk(const Nz::Vector3ui& indices) const
	{
		return *m_chunks[GetChunkIndex(indices)].chunk;
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

