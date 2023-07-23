// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Math/Box.hpp>

namespace tsom
{
	inline Chunk::Chunk(std::size_t width, std::size_t height, std::size_t depth, float cellSize) :
	m_depth(depth),
	m_height(height),
	m_width(width),
	m_cells(width * height * depth, VoxelBlock::Empty),
	m_cellSize(cellSize)
	{
	}

	inline VoxelBlock Chunk::GetCellContent(std::size_t x, std::size_t y, std::size_t z) const
	{
		return m_cells[m_width * (m_height * z + y) + x];
	}

	inline std::size_t tsom::Chunk::GetDepth() const
	{
		return m_depth;
	}

	inline std::size_t Chunk::GetHeight() const
	{
		return m_height;
	}

	inline std::optional<VoxelBlock> Chunk::GetNeighborCell(std::size_t x, std::size_t y, std::size_t z, int xOffset, int yOffset, int zOffset) const
	{
		const Chunk* currentChunk = this;
		if (xOffset < 0)
		{
			unsigned int offset = std::abs(xOffset);
			if (offset > x)
				return {};

			x -= offset;
		}
		else
		{
			x += xOffset;
			if (x >= m_width)
				return {};
		}

		if (yOffset < 0)
		{
			unsigned int offset = std::abs(yOffset);
			if (offset > y)
				return {};

			y -= offset;
		}
		else
		{
			y += yOffset;
			if (y >= m_height)
				return {};
		}

		if (zOffset < 0)
		{
			unsigned int offset = std::abs(zOffset);
			if (offset > z)
				return {};

			z -= offset;
		}
		else
		{
			z += zOffset;
			if (z >= m_height)
				return {};
		}

		/*while (zOffset > 0)
		{
			const NeighborGrid& neighborGrid = m_neighborGrids[Direction::Up];
			if (!neighborGrid.grid)
				return {};

			currentChunk = neighborGrid.grid;
			x += neighborGrid.offset.x;
			y += neighborGrid.offset.y;
			if (x >= currentChunk->GetWidth() || y >= currentChunk->GetHeight())
				return {};

			zOffset--;
		}

		while (zOffset < 0)
		{
			const NeighborGrid& neighborGrid = m_neighborGrids[Direction::Down];
			if (!neighborGrid.grid)
				return {};

			currentChunk = neighborGrid.grid;
			x += neighborGrid.offset.x;
			y += neighborGrid.offset.y;
			if (x >= currentChunk->GetWidth() || y >= currentChunk->GetHeight())
				return {};

			zOffset++;
		}*/

		return currentChunk->GetCellContent(x, y, z);
	}

	inline std::size_t Chunk::GetWidth() const
	{
		return m_width;
	}

	inline void Chunk::UpdateCell(std::size_t x, std::size_t y, std::size_t z, VoxelBlock cellType)
	{
		m_cells[m_width * (m_height * z + y) + x] = cellType;
	}
}
