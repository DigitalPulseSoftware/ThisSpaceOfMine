// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_PLANET_HPP
#define TSOM_CLIENT_PLANET_HPP

#include <Client/VoxelGrid.hpp>
#include <vector>

namespace Nz
{
	class GraphicalMesh;
	class JoltCollider3D;
}

namespace tsom
{
	class Planet
	{
		public:
			Planet(std::size_t gridDims, float tileSize, float cornerRadius);
			Planet(const Planet&) = delete;
			Planet(Planet&&) = delete;
			~Planet() = default;

			std::shared_ptr<Nz::JoltCollider3D> BuildCollider();
			std::shared_ptr<Nz::GraphicalMesh> BuildGfxMesh();

			inline float GetCornerRadius() const;
			inline std::size_t GetGridDimensions() const;
			inline float GetTileSize() const;

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

		private:
			void BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices);
			void RebuildGrid();

			std::array<std::vector<std::unique_ptr<VoxelGrid>>, 6> m_grids;
			std::size_t m_gridDimensions;
			float m_tileSize;
			float m_cornerRadius;
	};
}

#include <Client/Planet.inl>

#endif // TSOM_CLIENT_VOXELGRID_HPP
