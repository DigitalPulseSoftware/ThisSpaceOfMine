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

			Chunk& AddChunk(const Nz::Vector3ui& indices);

			void GenerateChunks();

			inline Nz::Vector3f GetCenter() const;
			inline Chunk* GetChunk(std::size_t chunkIndex);
			inline const Chunk* GetChunk(std::size_t chunkIndex) const;
			inline Chunk& GetChunk(const Nz::Vector3ui& indices);
			inline const Chunk& GetChunk(const Nz::Vector3ui& indices) const;
			inline std::size_t GetChunkCount() const;
			inline std::size_t GetChunkIndex(const Nz::Vector3ui& indices) const;
			inline Chunk& GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos = nullptr);
			inline const Chunk& GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos = nullptr) const;
			inline Chunk* GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos = nullptr);
			inline const Chunk* GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos = nullptr) const;
			inline Nz::Vector3f GetChunkOffset(const Nz::Vector3ui& indices) const;
			inline float GetCornerRadius() const;
			inline Nz::Vector3ui GetGridDimensions() const;
			inline float GetTileSize() const;

			void RemoveChunk(const Nz::Vector3ui& indices);

			inline void UpdateCornerRadius(float cornerRadius);

			Planet& operator=(const Planet&) = delete;
			Planet& operator=(Planet&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

			NazaraSignal(OnChunkAdded, Planet* /*planet*/, Chunk* /*chunk*/);
			NazaraSignal(OnChunkRemove, Planet* /*planet*/, Chunk* /*chunk*/);
			NazaraSignal(OnChunkUpdated, Planet* /*planet*/, Chunk* /*chunk*/);

		protected:
			struct ChunkData
			{
				std::unique_ptr<Chunk> chunk;

				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
			};

			std::vector<ChunkData> m_chunks;
			Nz::Vector3ui m_chunkCount;
			Nz::Vector3ui m_gridSize;
			float m_tileSize;
			float m_cornerRadius;
	};
}

#include <CommonLib/Planet.inl>

#endif // TSOM_COMMONLIB_VOXELGRID_HPP
