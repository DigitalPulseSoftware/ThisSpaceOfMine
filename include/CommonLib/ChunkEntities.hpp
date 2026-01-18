// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKENTITIES_HPP
#define TSOM_COMMONLIB_CHUNKENTITIES_HPP

#include <CommonLib/ChunkContainer.hpp>
#include <Nazara/Core/Node.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>
#include <atomic>
#include <mutex>
#include <vector>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
}

namespace tsom
{
	class BlockLibrary;

	class TSOM_COMMONLIB_API ChunkEntities
	{
		public:
			ChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, std::size_t layerIndex);
			ChunkEntities(const ChunkEntities&) = delete;
			ChunkEntities(ChunkEntities&&) = delete;
			~ChunkEntities();

			void ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, entt::handle chunkEntity)> callback);

			void SetParentEntity(entt::handle entity);

			void Update();

			ChunkEntities& operator=(const ChunkEntities&) = delete;
			ChunkEntities& operator=(ChunkEntities&&) = delete;

		protected:
			struct NoInit {};
			struct UpdateJob;

			ChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, std::size_t layerIndex, NoInit);

			void CreateChunkEntity(const ChunkIndices& chunkIndices, Chunk& chunk);
			void DestroyChunkEntity(const ChunkIndices& chunkIndices);
			void FillChunks();
			virtual UpdateJob* ProcessChunkUpdate(const Chunk& chunk, DirectionMask neighborMask);
			void OnParentNodeInvalidated(const Nz::Node* node);
			inline void UpdateChunkEntity(const ChunkIndices& chunkIndices, DirectionMask neighborMask);

			struct UpdateJob
			{
				std::function<void(const ChunkIndices& chunkIndices, UpdateJob&& job)> applyFunc;
				std::atomic_bool cancelled = false;
				std::atomic_uint jobDone = 0;
				Nz::FixedVector<ChunkIndices, 3 * 3 * 3> chunkDependencies; //< 27 to be able to have all neighbor
				unsigned int taskCount;

				inline bool HasFinished() const
				{
					return jobDone == taskCount;
				}
			};

			struct ColliderUpdateJob : UpdateJob
			{
				std::shared_ptr<Nz::Collider3D> collider;
			};

			struct FinishedJob
			{
				ChunkIndices chunkIndices;
				std::shared_ptr<UpdateJob> job;
			};

			NazaraSlot(ChunkContainer, OnChunkLayerAdded, m_onChunkAdded);
			NazaraSlot(ChunkContainer, OnChunkLayerRemove, m_onChunkRemove);
			NazaraSlot(ChunkContainer, OnChunkUpdated, m_onChunkUpdated);
			NazaraSlot(Nz::Node, OnNodeInvalidation, m_onParentNodeInvalidated);

			std::mutex m_chunkLock;
			std::mutex m_invalidatedChunkMutex;
			std::size_t m_layerIndex;
			std::vector<FinishedJob> m_finishedJobs;
			entt::handle m_parentEntity;
			tsl::hopscotch_map<ChunkIndices, DirectionMask> m_invalidatedChunks;
			tsl::hopscotch_map<ChunkIndices, std::shared_ptr<UpdateJob>> m_updateJobs;
			tsl::hopscotch_map<ChunkIndices, entt::handle> m_chunkEntities;
			Nz::ApplicationBase& m_application;
			Nz::EnttWorld& m_world;
			const BlockLibrary& m_blockLibrary;
			ChunkContainer& m_chunkContainer;
	};
}

#include <CommonLib/ChunkEntities.inl>

#endif // TSOM_COMMONLIB_CHUNKENTITIES_HPP
