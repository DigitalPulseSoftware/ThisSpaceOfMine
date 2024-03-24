// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Error.hpp>

namespace tsom
{
	inline void Ship::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback)
	{
		callback(ChunkIndices{ 0, 0, 0 }, m_chunk);
	}

	inline void Ship::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const
	{
		callback(ChunkIndices{ 0, 0, 0}, m_chunk);
	}

	inline Nz::Vector3f Ship::GetCenter() const
	{
		return Nz::Vector3f::Zero();
	}

	inline Chunk& Ship::GetChunk()
	{
		return m_chunk;
	}

	inline const Chunk& Ship::GetChunk() const
	{
		return m_chunk;
	}

	inline Chunk* Ship::GetChunk(const ChunkIndices& chunkIndices)
	{
		NazaraAssert(chunkIndices == ChunkIndices(0, 0, 0), "invalid indices");
		return &m_chunk;
	}

	inline const Chunk* Ship::GetChunk(const ChunkIndices& chunkIndices) const
	{
		NazaraAssert(chunkIndices == ChunkIndices(0, 0, 0), "invalid indices");
		return &m_chunk;
	}

	inline std::size_t Ship::GetChunkCount() const
	{
		return 1;
	}
}
