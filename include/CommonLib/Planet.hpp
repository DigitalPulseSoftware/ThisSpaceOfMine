// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_PLANET_HPP
#define TSOM_COMMONLIB_PLANET_HPP

#include <CommonLib/Chunk.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/Export.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <Nazara/Utility/VertexStruct.hpp>
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
			Planet(const Nz::Vector3ui& gridSize, float tileSize, float cornerRadius);
			Planet(const Planet&) = delete;
			Planet(Planet&&) = delete;
			~Planet() = default;

			std::shared_ptr<Nz::JoltCollider3D> BuildCollider();

			std::optional<Nz::Vector3ui> ComputeGridCell(const Nz::Vector3f& position) const;

			inline Nz::Vector3f GetCenter() const;
			inline Chunk& GetChunk(const Nz::Vector3ui& indices);
			inline const Chunk& GetChunk(const Nz::Vector3ui& indices) const;
			inline Chunk& GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos = nullptr);
			inline const Chunk& GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos = nullptr) const;
			inline Chunk* GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos = nullptr);
			inline const Chunk* GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos = nullptr) const;
			inline Nz::Vector3f GetChunkOffset(const Nz::Vector3ui& indices) const;
			inline float GetCornerRadius() const;
			inline Nz::Vector3ui GetGridDimensions() const;
			inline float GetTileSize() const;

			inline void UpdateCornerRadius(float cornerRadius);

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		protected:
			void BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices);
			void RebuildGrid();

			std::vector<std::unique_ptr<Chunk>> m_chunks;
			Nz::Vector3ui m_chunkCount;
			Nz::Vector3ui m_gridSize;
			float m_tileSize;
			float m_cornerRadius;
	};
}

#include <CommonLib/Planet.inl>

#endif // TSOM_COMMONLIB_VOXELGRID_HPP
