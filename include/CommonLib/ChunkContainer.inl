// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cmath>

namespace tsom
{
	inline ChunkContainer::ChunkContainer(float tileSize) :
	m_tileSize(tileSize)
	{
	}

	inline BlockIndices ChunkContainer::GetBlockIndices(const ChunkIndices& chunkIndices, const Nz::Vector3ui& indices) const
	{
		return chunkIndices * Nz::Int32(ChunkSize) + BlockIndices(indices.x, indices.z, indices.y) - BlockIndices(Nz::Int32(ChunkSize)) / 2;
	}

	inline ChunkIndices ChunkContainer::GetChunkIndicesByBlockIndices(const BlockIndices& indices, Nz::Vector3ui* localIndices) const
	{
		BlockIndices adjustedIndices = indices + BlockIndices(Nz::Int32(ChunkSize)) / 2;

		// Correctly map negative indices to the right values
		BlockIndices adjustedIndicesOffseted = BlockIndices::Apply(adjustedIndices, [](int val) -> int
		{
			return (val < 0) ? val - int(ChunkSize) + 1 : val;
		});

		ChunkIndices chunkIndices = adjustedIndicesOffseted / Nz::Int32(ChunkSize);
		if (localIndices)
		{
			Nz::Vector3i localBlockIndices = adjustedIndices - chunkIndices * Nz::Int32(ChunkSize);
			localBlockIndices = Nz::Vector3i::Apply(localBlockIndices, [](int val)
			{
				return (val < 0) ? val + int(ChunkSize) : val;
			});

			*localIndices = Nz::Vector3ui(Nz::SafeCast<unsigned int>(localBlockIndices.x), Nz::SafeCast<unsigned int>(localBlockIndices.z), Nz::SafeCast<unsigned int>(localBlockIndices.y));
		}

		return chunkIndices;
	}

	inline ChunkIndices ChunkContainer::GetChunkIndicesByPosition(const Nz::Vector3f& position) const
	{
		Nz::Vector3f relativePos = position - GetCenter();
		Nz::Vector3f chunkSize(ChunkSize * m_tileSize);

		Nz::Vector3f indices = Nz::Vector3f::Apply((relativePos + chunkSize * 0.5f) / chunkSize - Nz::Vector3f(0.5f), std::roundf);
		return ChunkIndices(indices.x, indices.y, indices.z);
	}

	inline Nz::Vector3f ChunkContainer::GetChunkOffset(const ChunkIndices& indices) const
	{
		return Nz::Vector3f(indices.x, indices.y, indices.z) * ChunkSize * m_tileSize;
	}

	inline float ChunkContainer::GetTileSize() const
	{
		return m_tileSize;
	}
}
