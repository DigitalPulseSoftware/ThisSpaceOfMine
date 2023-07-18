// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Math/Box.hpp>

namespace tsom
{
	inline void VoxelGrid::AttachNeighborGrid(Direction direction, const Nz::Vector2i& gridOffset, VoxelGrid* grid)
	{
		m_neighborGrids[direction].grid = grid;
		m_neighborGrids[direction].offset = gridOffset;
	}

	inline Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> VoxelGrid::ComputeVoxelCorners(std::size_t x, std::size_t y, float cellSize) const
	{
		float fX = x * cellSize;
		float fY = y * cellSize;

		float heightOffset = m_height * cellSize * 0.5f;

		Nz::Boxf box(fX - heightOffset, 0.f, fY - heightOffset, cellSize, cellSize, cellSize);
		Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> corners {
			box.GetCorner(Nz::BoxCorner::FarLeftBottom),
				box.GetCorner(Nz::BoxCorner::FarLeftTop),
				box.GetCorner(Nz::BoxCorner::FarRightBottom),
				box.GetCorner(Nz::BoxCorner::FarRightTop),
				box.GetCorner(Nz::BoxCorner::NearLeftBottom),
				box.GetCorner(Nz::BoxCorner::NearLeftTop),
				box.GetCorner(Nz::BoxCorner::NearRightBottom),
				box.GetCorner(Nz::BoxCorner::NearRightTop)
		};

		if (x == 0)
		{
			corners[Nz::BoxCorner::NearLeftTop].x -= cellSize;
			corners[Nz::BoxCorner::FarLeftTop].x -= cellSize;
		}

		if (x == m_width - 1)
		{
			corners[Nz::BoxCorner::NearRightTop].x += cellSize;
			corners[Nz::BoxCorner::FarRightTop].x += cellSize;
		}

		if (y == 0)
		{
			corners[Nz::BoxCorner::FarLeftTop].z -= cellSize;
			corners[Nz::BoxCorner::FarRightTop].z -= cellSize;
		}

		if (y == m_height - 1)
		{
			corners[Nz::BoxCorner::NearLeftTop].z += cellSize;
			corners[Nz::BoxCorner::NearRightTop].z += cellSize;
		}

		return corners;
	}

	inline VoxelBlock VoxelGrid::GetCellContent(std::size_t x, std::size_t y) const
	{
		return m_cells[y * m_width + x];
	}

	inline std::size_t VoxelGrid::GetHeight() const
	{
		return m_height;
	}

	inline std::optional<VoxelBlock> VoxelGrid::GetNeighborCell(std::size_t x, std::size_t y, int xOffset, int yOffset, int zOffset) const
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

	inline void VoxelGrid::UpdateCell(std::size_t x, std::size_t y, VoxelBlock cellType)
	{
		m_cells[y * m_width + x] = cellType;
	}
}

#include <Nazara/Widgets/DebugOff.hpp>

