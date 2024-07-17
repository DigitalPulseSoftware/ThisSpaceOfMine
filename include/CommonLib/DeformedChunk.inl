// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline DeformedChunk::DeformedChunk(ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float cellSize, const Nz::Vector3f& deformationCenter, float deformationRadius) :
	Chunk(owner, indices, size, cellSize),
	m_deformationCenter(deformationCenter),
	m_deformationRadius(deformationRadius)
	{
	}

	inline void DeformedChunk::UpdateDeformationRadius(float deformationRadius)
	{
		m_deformationRadius = deformationRadius;
	}
}
