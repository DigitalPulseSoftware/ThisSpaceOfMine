// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Widgets/Debug.hpp>

namespace tsom
{
	inline void VoxelGrid::AttachNeighborGrid(Direction direction, const Nz::Vector2i& gridOffset, VoxelGrid* grid)
	{
		m_neighborGrids[direction].grid = grid;
		m_neighborGrids[direction].offset = gridOffset;
	}

	inline VoxelCell VoxelGrid::GetCellContent(std::size_t x, std::size_t y) const
	{
		return m_cells[y * m_width + x];
	}

	inline std::size_t VoxelGrid::GetHeight() const
	{
		return m_height;
	}

	inline std::optional<VoxelCell> VoxelGrid::GetNeighborCell(std::size_t x, std::size_t y, int xOffset, int yOffset, int zOffset) const
	{
		const VoxelGrid* currentGrid = this;
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

		while (zOffset > 0)
		{
			const NeighborGrid& neighborGrid = m_neighborGrids[Direction::Up];
			if (!neighborGrid.grid)
				return {};

			currentGrid = neighborGrid.grid;
			x += neighborGrid.offset.x;
			y += neighborGrid.offset.y;
			if (x >= currentGrid->GetWidth() || y >= currentGrid->GetHeight())
				return {};

			zOffset--;
		}

		while (zOffset < 0)
		{
			const NeighborGrid& neighborGrid = m_neighborGrids[Direction::Down];
			if (!neighborGrid.grid)
				return {};

			currentGrid = neighborGrid.grid;
			x += neighborGrid.offset.x;
			y += neighborGrid.offset.y;
			if (x >= currentGrid->GetWidth() || y >= currentGrid->GetHeight())
				return {};

			zOffset++;
		}

		return currentGrid->GetCellContent(x, y);
	}

	inline std::size_t VoxelGrid::GetWidth() const
	{
		return m_width;
	}
}

#include <Nazara/Widgets/DebugOff.hpp>

