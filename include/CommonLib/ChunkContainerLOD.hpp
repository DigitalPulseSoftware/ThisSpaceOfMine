// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKCONTAINERLOD_HPP
#define TSOM_COMMONLIB_CHUNKCONTAINERLOD_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>
#include <memory>
#include <mutex>

namespace tsom
{
	class TSOM_COMMONLIB_API ChunkContainerLOD : public ChunkContainer
	{
		public:
			ChunkContainerLOD(const ChunkContainer& parentChunkContainer);
			ChunkContainerLOD(const ChunkContainerLOD&) = delete;
			ChunkContainerLOD(ChunkContainerLOD&&) = delete;
			~ChunkContainerLOD() = default;

			Chunk& AddChunk(const ChunkIndices& indices, const Nz::FunctionRef<void(BlockIndex* blocks)>& initCallback = nullptr) override;

			void ClearChunks() override;

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, Chunk& chunk)> callback) override;
			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, const Chunk& chunk)> callback) const override;

			inline Nz::Vector3f GetCenter() const override;
			inline Chunk* GetChunk(const ChunkIndices& chunkIndices) override;
			inline const Chunk* GetChunk(const ChunkIndices& chunkIndices) const override;
			inline std::size_t GetChunkCount() const override;

			void RemoveChunk(const ChunkIndices& indices) override;

			void Update();

			ChunkContainerLOD& operator=(const ChunkContainerLOD&) = delete;
			ChunkContainerLOD& operator=(ChunkContainerLOD&&) = delete;

			static constexpr unsigned int ChunkSize = 32;

		private:
			using NeighborChunkArray = std::array<const Chunk*, 8>;

			BlockIndex ComputeBlockIndex(const NeighborChunkArray& neighborChunkArray, const Chunk& chunkLOD, const Nz::Vector3ui& blockIndicesLOD);
			void OnParentChunkAdded(const Chunk* parentChunk);
			void OnParentChunkBlockUpdated(Chunk* parentChunk, const Nz::Vector3ui& indices, BlockIndex oldBlock, BlockIndex newBlock, std::size_t prevLayerIndex, std::size_t newLayerIndex);
			void OnParentChunkCleared(Chunk* parentChunk, Nz::UInt32 previousActiveLayerMask);
			void OnParentChunkRemove(Chunk* parentChunk);
			void OnParentChunkReset(Chunk* parentChunk);
			void RebuildChunkLOD(Chunk& chunkLOD);

			static inline ChunkIndices ToLODChunkIndices(const ChunkIndices& parentChunkIndices);
			static inline ChunkIndices ToParentChunkIndices(const ChunkIndices& lodChunkIndices);
			static inline std::size_t ToNeighborChunkIndices(const Nz::Vector3ui& offset);

			using ChunkInvalidationData = Nz::HybridVector<Nz::Vector3ui, 3>; //< empty = full chunk rebuild
			static constexpr std::size_t FullRebuildThreshold = 8; //< if more than this number of block are changed, a full chunk rebuild is preferred

			struct ChunkData
			{
				std::shared_ptr<Chunk> chunk;

				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
				NazaraSlot(Chunk, OnClear, onClear);
				NazaraSlot(Chunk, OnLayerRegistered, onLayerRegistered);
				NazaraSlot(Chunk, OnLayerUnregistered, onLayerUnregistered);
				NazaraSlot(Chunk, OnReset, onReset);
			};

			struct ParentChunkData
			{
				NazaraSlot(Chunk, OnBlockUpdated, onUpdated);
				NazaraSlot(Chunk, OnClear, onClear);
				NazaraSlot(Chunk, OnReset, onReset);
			};

			std::mutex m_chunkLayerAddedSignalMutex;
			std::mutex m_chunkLayerRemovedSignalMutex;
			std::mutex m_chunkUpdatedSignalMutex;
			tsl::hopscotch_map<ChunkIndices, ChunkData> m_chunks;
			tsl::hopscotch_map<ChunkIndices, ParentChunkData> m_parentChunks;
			tsl::hopscotch_map<ChunkIndices, ChunkInvalidationData> m_invalidatedChunks;
			const ChunkContainer& m_parentChunkContainer;
	};
}

#include <CommonLib/ChunkContainerLOD.inl>

#endif // TSOM_COMMONLIB_CHUNKCONTAINERLOD_HPP
