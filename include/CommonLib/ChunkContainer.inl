// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline ChunkContainer::ChunkContainer(const Nz::Vector3ui& gridSize, float tileSize) :
	m_gridSize((gridSize + Nz::Vector3ui(ChunkSize - 1)) / Nz::Vector3ui(ChunkSize) * Nz::Vector3ui(ChunkSize)),
	m_tileSize(tileSize)
	{
		m_chunkCount = m_gridSize / ChunkSize;
	}

	inline std::size_t ChunkContainer::GetChunkIndex(const Nz::Vector3ui& indices) const
	{
		assert(indices.x < m_chunkCount.x);
		assert(indices.y < m_chunkCount.y);
		assert(indices.z < m_chunkCount.z);
		return indices.z * m_chunkCount.y * m_chunkCount.x + indices.y * m_chunkCount.x + indices.x;
	}

	inline Chunk& ChunkContainer::GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos)
	{
		if (innerPos)
			*innerPos = Nz::Vector3ui(gridPosition.x % ChunkSize, gridPosition.y % ChunkSize, gridPosition.z % ChunkSize);

		return GetChunk(gridPosition / ChunkSize);
	}

	inline const Chunk& ChunkContainer::GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos) const
	{
		if (innerPos)
			*innerPos = Nz::Vector3ui(gridPosition.x % ChunkSize, gridPosition.y % ChunkSize, gridPosition.z % ChunkSize);

		return GetChunk(gridPosition / ChunkSize);
	}

	inline Chunk* ChunkContainer::GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos)
	{
		Nz::Vector3f recenterOffset = 0.5f * Nz::Vector3f(m_gridSize) * m_tileSize;
		Nz::Vector3f offsetPos = position + recenterOffset;
		Nz::Vector3f chunkSize(ChunkSize * m_tileSize);

		Nz::Vector3f indices = offsetPos / Nz::Vector3f(ChunkSize * m_tileSize);
		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return nullptr;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_chunkCount.x || pos.y >= m_chunkCount.y || pos.z >= m_chunkCount.z)
			return nullptr;

		if (localPos)
			*localPos = { std::fmod(offsetPos.x, chunkSize.x), std::fmod(offsetPos.y, chunkSize.y), std::fmod(offsetPos.z, chunkSize.z) };

		return &GetChunk(pos);
	}

	inline const Chunk* ChunkContainer::GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos) const
	{
		Nz::Vector3f recenterOffset = 0.5f * Nz::Vector3f(m_gridSize) * m_tileSize;
		Nz::Vector3f offsetPos = position + recenterOffset;
		Nz::Vector3f chunkSize(ChunkSize * m_tileSize);

		Nz::Vector3f indices = offsetPos / Nz::Vector3f(ChunkSize * m_tileSize);
		if (indices.x < 0.f || indices.y < 0.f || indices.z < 0.f)
			return nullptr;

		Nz::Vector3ui pos(indices.x, indices.z, indices.y);
		if (pos.x >= m_chunkCount.x || pos.y >= m_chunkCount.y || pos.z >= m_chunkCount.z)
			return nullptr;

		if (localPos)
			*localPos = { std::fmod(offsetPos.x, chunkSize.x), std::fmod(offsetPos.y, chunkSize.y), std::fmod(offsetPos.z, chunkSize.z) };

		return &GetChunk(pos);
	}

	inline Nz::Vector3f ChunkContainer::GetChunkOffset(const Nz::Vector3ui& indices) const
	{
		// Recenter
		Nz::Vector3f offset = -0.5f * Nz::Vector3f(m_gridSize) * m_tileSize - GetCenter() * 0.5f;

		// Chunk offset
		offset += Nz::Vector3f(indices.x, indices.z, indices.y) * ChunkSize * m_tileSize;

		return offset;
	}

	inline Nz::Vector3ui ChunkContainer::GetGridDimensions() const
	{
		return m_gridSize;
	}

	inline float ChunkContainer::GetTileSize() const
	{
		return m_tileSize;
	}
}

