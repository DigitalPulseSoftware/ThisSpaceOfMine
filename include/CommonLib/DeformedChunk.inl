// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline DeformedChunk::DeformedChunk(const BlockLibrary& blockLibrary, ChunkContainer& owner, const ChunkIndices& indices, const Nz::Vector3ui& size, float cellSize, const Nz::Vector3f& deformationCenter, float deformationRadius) :
	Chunk(blockLibrary, owner, indices, size, cellSize),
	m_deformationCenter(deformationCenter),
	m_deformationRadius(deformationRadius)
	{
	}

	inline void DeformedChunk::UpdateDeformationRadius(float deformationRadius)
	{
		m_deformationRadius = deformationRadius;
	}

	inline Nz::Vector3f DeformedChunk::DeformPosition(const Nz::Vector3f& position, const Nz::Vector3f& deformationCenter, float deformationRadius)
	{
		float distToCenter = std::max({
			std::abs(position.x - deformationCenter.x),
			std::abs(position.y - deformationCenter.y),
			std::abs(position.z - deformationCenter.z),
		});

		float innerReductionSize = std::max(distToCenter - deformationRadius, 0.f);
		Nz::Boxf innerBox(deformationCenter - Nz::Vector3f(innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

		Nz::Vector3f innerPos = Nz::Vector3f::Clamp(position, innerBox.GetMinimum(), innerBox.GetMaximum());
		Nz::Vector3f normal = Nz::Vector3f::Normalize(position - innerPos);

		return innerPos + normal * std::min(deformationRadius, distToCenter);
	}
}
