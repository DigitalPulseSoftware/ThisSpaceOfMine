// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline DeformedChunk::DeformedChunk(std::size_t width, std::size_t height, std::size_t depth, float cellSize, const Nz::Vector3f& deformationCenter, float deformationRadius) :
	Chunk(width, height, depth, cellSize),
	m_deformationCenter(deformationCenter),
	m_deformationRadius(deformationRadius)
	{
	}

	inline void DeformedChunk::UpdateDeformationRadius(float deformationRadius)
	{
		m_deformationRadius = deformationRadius;
	}
}
