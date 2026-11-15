// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "ChunkContainerLOD.hpp"
// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::Vector3f ChunkContainerLOD::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline Chunk* ChunkContainerLOD::GetChunk(const ChunkIndices& chunkIndices)
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline const Chunk* ChunkContainerLOD::GetChunk(const ChunkIndices& chunkIndices) const
	{
		auto it = m_chunks.find(chunkIndices);
		if (it == m_chunks.end())
			return nullptr;

		return it->second.chunk.get();
	}

	inline std::size_t ChunkContainerLOD::GetChunkCount() const
	{
		return m_chunks.size();
	}

	inline ChunkIndices ChunkContainerLOD::ToLODChunkIndices(const ChunkIndices& parentChunkIndices)
	{
		// Mapping of parent to LOD: [-4;-3] => -2 [-2,-1] => -1 [0;1] => 0, [2;3] => 1
		return ChunkIndices::Apply(parentChunkIndices, [](Nz::Int32 i) { return i >= 0 ? i : i - 1; }) / 2;
	}

	inline ChunkIndices ChunkContainerLOD::ToParentChunkIndices(const ChunkIndices& lodChunkIndices)
	{
		// Mapping of LOD to parent: -2 => -4, -1 => -2, 0 => 0, 1 => 2
		return lodChunkIndices * 2;
	}

	inline std::size_t ChunkContainerLOD::ToNeighborChunkIndices(const Nz::Vector3ui& offset)
	{
		assert(offset.x >= 0 && offset.x <= 1);
		assert(offset.y >= 0 && offset.y <= 1);
		assert(offset.z >= 0 && offset.z <= 1);
		return offset.z << 2 | offset.y << 1 | offset.x;
	}
}
