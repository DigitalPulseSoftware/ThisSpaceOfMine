// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline DeformedChunk::DeformedChunk(const Nz::Vector3ui& indices, const Nz::Vector3ui& size, float cellSize, const Nz::Vector3f& deformationCenter, float deformationRadius) :
	Chunk(indices, size, cellSize),
	m_deformationCenter(deformationCenter),
	m_deformationRadius(deformationRadius)
	{
	}

	inline void DeformedChunk::UpdateDeformationRadius(float deformationRadius)
	{
		m_deformationRadius = deformationRadius;
	}
}
