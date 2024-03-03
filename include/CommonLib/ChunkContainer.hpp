// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKCONTAINER_HPP
#define TSOM_COMMONLIB_CHUNKCONTAINER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <NazaraUtils/Signal.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ChunkContainer
	{
		public:
			inline ChunkContainer(float tileSize);
			ChunkContainer(const ChunkContainer&) = delete;
			ChunkContainer(ChunkContainer&&) = delete;
			virtual ~ChunkContainer();

			virtual void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) = 0;
			virtual void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const = 0;

			inline BlockIndices GetBlockIndices(const ChunkIndices& chunkIndices, const Nz::Vector3ui& indices) const;
			virtual Nz::Vector3f GetCenter() const = 0;
			virtual Chunk* GetChunk(const ChunkIndices& chunkIndices) = 0;
			virtual const Chunk* GetChunk(const ChunkIndices& chunkIndex) const = 0;
			virtual std::size_t GetChunkCount() const = 0;
			inline ChunkIndices GetChunkIndicesByBlockIndices(const BlockIndices& indices, Nz::Vector3ui* localIndices = nullptr) const;
			inline ChunkIndices GetChunkIndicesByPosition(const Nz::Vector3f& position) const;
			inline Nz::Vector3f GetChunkOffset(const ChunkIndices& indices) const;
			inline float GetTileSize() const;

			ChunkContainer& operator=(const ChunkContainer&) = delete;
			ChunkContainer& operator=(ChunkContainer&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

			NazaraSignal(OnChunkAdded, ChunkContainer* /*planet*/, Chunk* /*chunk*/);
			NazaraSignal(OnChunkRemove, ChunkContainer* /*planet*/, Chunk* /*chunk*/);
			NazaraSignal(OnChunkUpdated, ChunkContainer* /*planet*/, Chunk* /*chunk*/);

		protected:
			float m_tileSize;
	};
}

#include <CommonLib/ChunkContainer.inl>

#endif // TSOM_COMMONLIB_CHUNKCONTAINER_HPP
