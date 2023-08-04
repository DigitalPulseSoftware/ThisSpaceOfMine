// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Math/Box.hpp>

namespace tsom
{
	inline Chunk::Chunk(const Nz::Vector3ui& indices, const Nz::Vector3ui& size, float cellSize) :
	m_cells(size.x * size.y * size.z, VoxelBlock::Empty),
	m_indices(indices),
	m_size(size),
	m_cellSize(cellSize)
	{
	}

	inline VoxelBlock Chunk::GetCellContent(const Nz::Vector3ui& indices) const
	{
		return m_cells[m_size.x * (m_size.y * indices.z + indices.y) + indices.x];
	}

	inline std::optional<VoxelBlock> Chunk::GetNeighborCell(Nz::Vector3ui indices, const Nz::Vector3i& offsets) const
	{
		const Chunk* currentChunk = this;
		if (offsets.x < 0)
		{
			unsigned int offset = std::abs(offsets.x);
			if (offset > indices.x)
				return {};

			indices.x -= offset;
		}
		else
		{
			indices.x += offsets.x;
			if (indices.x >= m_size.x)
				return {};
		}

		if (offsets.y < 0)
		{
			unsigned int offset = std::abs(offsets.y);
			if (offset > indices.y)
				return {};

			indices.y -= offset;
		}
		else
		{
			indices.y += offsets.y;
			if (indices.y >= m_size.y)
				return {};
		}

		if (offsets.z < 0)
		{
			unsigned int offset = std::abs(offsets.z);
			if (offset > indices.z)
				return {};

			indices.z -= offset;
		}
		else
		{
			indices.z += offsets.z;
			if (indices.z >= m_size.z)
				return {};
		}

		/*while (offsets.z > 0)
		{
			const NeighborGrid& neighborGrid = m_neighborGrids[Direction::Up];
			if (!neighborGrid.grid)
				return {};

			currentChunk = neighborGrid.grid;
			x += neighborGrid.offsets.x;
			y += neighborGrid.offsets.y;
			if (x >= currentChunk->GetWidth() || y >= currentChunk->GetHeight())
				return {};

			offsets.z--;
		}

		while (offsets.z < 0)
		{
			const NeighborGrid& neighborGrid = m_neighborGrids[Direction::Down];
			if (!neighborGrid.grid)
				return {};

			currentChunk = neighborGrid.grid;
			x += neighborGrid.offsets.x;
			y += neighborGrid.offsets.y;
			if (x >= currentChunk->GetWidth() || y >= currentChunk->GetHeight())
				return {};

			offsets.z++;
		}*/

		return currentChunk->GetCellContent(indices);
	}

	inline const Nz::Vector3ui& Chunk::GetIndices() const
	{
		return m_indices;
	}
	
	inline const Nz::Vector3ui& Chunk::GetSize() const
	{
		return m_size;
	}

	inline void Chunk::UpdateCell(const Nz::Vector3ui& indices, VoxelBlock cellType)
	{
		m_cells[m_size.x * (m_size.y * indices.z + indices.y) + indices.x] = cellType;
	}
}
