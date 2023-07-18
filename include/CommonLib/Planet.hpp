// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_PLANET_HPP
#define TSOM_COMMONLIB_PLANET_HPP

#include <CommonLib/Direction.hpp>
#include <CommonLib/Export.hpp>
#include <CommonLib/VoxelGrid.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <memory>
#include <tuple>
#include <vector>

namespace Nz
{
	class DebugDrawer;
	class JoltCollider3D;
}

namespace tsom
{
	class TSOM_COMMONLIB_API Planet
	{
		public:
			struct GridCellIntersection
			{
				VoxelGrid* targetGrid;
				Direction direction;
				std::size_t cellX;
				std::size_t cellY;
				float gridHeight;
			};

			Planet(std::size_t gridDims, float tileSize, float cornerRadius);
			Planet(const Planet&) = delete;
			Planet(Planet&&) = delete;
			~Planet() = default;

			std::shared_ptr<Nz::JoltCollider3D> BuildCollider();

			std::optional<GridCellIntersection> ComputeGridCell(const Nz::Vector3f& position);

			Nz::Vector3f DeformPosition(const Nz::Vector3f& position);

			inline Nz::Vector3f GetCenter() const;
			inline float GetCornerRadius() const;
			inline std::size_t GetGridDimensions() const;
			inline float GetTileSize() const;

			inline void UpdateCornerRadius(float cornerRadius);

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

		protected:
			void BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices);
			void RebuildGrid();

			Nz::EnumArray<Direction, std::vector<std::unique_ptr<VoxelGrid>>> m_grids;
			std::size_t m_gridDimensions;
			float m_tileSize;
			float m_cornerRadius;
	};
}

#include <CommonLib/Planet.inl>

#endif // TSOM_COMMONLIB_VOXELGRID_HPP
