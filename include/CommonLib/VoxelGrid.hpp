// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_VOXELGRID_HPP
#define TSOM_COMMONLIB_VOXELGRID_HPP

#include <CommonLib/Direction.hpp>
#include <CommonLib/VoxelBlock.hpp>
#include <Nazara/Core/Color.hpp>
#include <Nazara/Math/Matrix4.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace Nz
{
	class StaticMesh;
	struct VertexStruct_XYZ_Color_UV;
}

namespace tsom
{
	class VoxelGrid
	{
		public:
			VoxelGrid(std::size_t width, std::size_t height, VoxelBlock defaultCell);
			VoxelGrid(const VoxelGrid&) = delete;
			VoxelGrid(VoxelGrid&&) = delete;
			~VoxelGrid() = default;

			inline void AttachNeighborGrid(Direction direction, const Nz::Vector2i& gridOffset, VoxelGrid* grid);

			void BuildMesh(const Nz::Matrix4f& transformMatrix, float cellSize, const Nz::Color& color, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices);

			inline Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(std::size_t x, std::size_t y, float cellSize) const;

			inline VoxelBlock GetCellContent(std::size_t x, std::size_t y) const;
			inline std::size_t GetHeight() const;
			inline std::optional<VoxelBlock> GetNeighborCell(std::size_t x, std::size_t y, int xOffset, int yOffset, int zOffset) const;
			inline std::size_t GetWidth() const;

			inline void UpdateCell(std::size_t x, std::size_t y, VoxelBlock cellType);

			VoxelGrid& operator=(const VoxelGrid&) = delete;
			VoxelGrid& operator=(VoxelGrid&&) = delete;

		private:
			struct NeighborGrid
			{
				VoxelGrid* grid = nullptr;
				Nz::Vector2i offset;
			};

			Nz::EnumArray<Direction, NeighborGrid> m_neighborGrids;
			std::size_t m_height;
			std::size_t m_width;
			std::vector<VoxelBlock> m_cells;
	};
}

#include <CommonLib/VoxelGrid.inl>

#endif // TSOM_COMMONLIB_VOXELGRID_HPP
