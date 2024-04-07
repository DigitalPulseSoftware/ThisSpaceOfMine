// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Error.hpp>

namespace tsom
{
	inline Nz::Vector3f Ship::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline FlatChunk* Ship::GetChunk(const ChunkIndices& chunkIndices)
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline const FlatChunk* Ship::GetChunk(const ChunkIndices& chunkIndices) const
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline std::size_t Ship::GetChunkCount() const
	{
		return m_chunks.size();
	}

	inline void Ship::UpdateUpDirection(const Nz::Vector3f& upDirection)
	{
		m_upDirection = upDirection;
	}
}
