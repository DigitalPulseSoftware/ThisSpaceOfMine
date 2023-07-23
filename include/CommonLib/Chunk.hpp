// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_CHUNK_HPP
#define TSOM_COMMONLIB_CHUNK_HPP

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
	class JoltCollider3D;
	struct VertexStruct_XYZ_Color_UV;
}

namespace tsom
{
	class Chunk
	{
		public:
			inline Chunk(std::size_t width, std::size_t height, std::size_t depth, float cellSize);
			Chunk(const Chunk&) = delete;
			Chunk(Chunk&&) = delete;
			~Chunk() = default;

			virtual std::shared_ptr<Nz::JoltCollider3D> BuildCollider(const Nz::Matrix4f& transformMatrix) const = 0;
			virtual void BuildMesh(const Nz::Matrix4f& transformMatrix, const Nz::Color& color, std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices) const;

			virtual std::optional<Nz::Vector3ui> ComputeCoordinates(const Nz::Vector3f& position) const = 0;
			virtual Nz::EnumArray<Nz::BoxCorner, Nz::Vector3f> ComputeVoxelCorners(std::size_t x, std::size_t y, std::size_t z) const = 0;

			inline VoxelBlock GetCellContent(std::size_t x, std::size_t y, std::size_t z) const;
			inline std::size_t GetDepth() const;
			inline std::size_t GetHeight() const;
			inline std::optional<VoxelBlock> GetNeighborCell(std::size_t x, std::size_t y, std::size_t z, int xOffset, int yOffset, int zOffset) const;
			inline std::size_t GetWidth() const;

			inline void UpdateCell(std::size_t x, std::size_t y, std::size_t z, VoxelBlock cellType);

			Chunk& operator=(const Chunk&) = delete;
			Chunk& operator=(Chunk&&) = delete;

		protected:
			std::size_t m_depth;
			std::size_t m_height;
			std::size_t m_width;
			std::vector<VoxelBlock> m_cells;
			float m_cellSize;
	};
}

#include <CommonLib/Chunk.inl>

#endif // TSOM_COMMONLIB_CHUNK_HPP
