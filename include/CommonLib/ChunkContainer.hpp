// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKCONTAINER_HPP
#define TSOM_COMMONLIB_CHUNKCONTAINER_HPP

#include <CommonLib/Chunk.hpp>
#include <CommonLib/Export.hpp>
#include <NazaraUtils/Signal.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace Nz
{
	class Collider3D;
}

namespace tsom
{
	class TSOM_COMMONLIB_API ChunkContainer
	{
		public:
			inline ChunkContainer(const Nz::Vector3ui& gridSize, float tileSize);
			ChunkContainer(const ChunkContainer&) = delete;
			ChunkContainer(ChunkContainer&&) = delete;
			virtual ~ChunkContainer();

			virtual Nz::Vector3f GetCenter() const = 0;
			virtual Chunk* GetChunk(std::size_t chunkIndex) = 0;
			virtual const Chunk* GetChunk(std::size_t chunkIndex) const = 0;
			virtual Chunk& GetChunk(const Nz::Vector3ui& indices) = 0;
			virtual const Chunk& GetChunk(const Nz::Vector3ui& indices) const = 0;
			virtual std::size_t GetChunkCount() const = 0;
			inline std::size_t GetChunkIndex(const Nz::Vector3ui& indices) const;
			inline Chunk& GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos = nullptr);
			inline const Chunk& GetChunkByIndices(const Nz::Vector3ui& gridPosition, Nz::Vector3ui* innerPos = nullptr) const;
			inline Chunk* GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos = nullptr);
			inline const Chunk* GetChunkByPosition(const Nz::Vector3f& position, Nz::Vector3f* localPos = nullptr) const;
			inline Nz::Vector3f GetChunkOffset(const Nz::Vector3ui& indices) const;
			inline Nz::Vector3ui GetGridDimensions() const;
			inline float GetTileSize() const;

			ChunkContainer& operator=(const ChunkContainer&) = delete;
			ChunkContainer& operator=(ChunkContainer&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

			NazaraSignal(OnChunkAdded, ChunkContainer* /*planet*/, Chunk* /*chunk*/);
			NazaraSignal(OnChunkRemove, ChunkContainer* /*planet*/, Chunk* /*chunk*/);
			NazaraSignal(OnChunkUpdated, ChunkContainer* /*planet*/, Chunk* /*chunk*/);

		protected:
			Nz::Vector3ui m_chunkCount;
			Nz::Vector3ui m_gridSize;
			float m_tileSize;
	};
}

#include <CommonLib/ChunkContainer.inl>

#endif // TSOM_COMMONLIB_VOXELGRID_HPP
