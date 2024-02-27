// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKENTITIES_HPP
#define TSOM_COMMONLIB_CHUNKENTITIES_HPP

#include <CommonLib/ChunkContainer.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <atomic>
#include <vector>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
}

namespace tsom
{
	class BlockLibrary;
	class Chunk;

	class TSOM_COMMONLIB_API ChunkEntities
	{
		public:
			ChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary);
			ChunkEntities(const ChunkEntities&) = delete;
			ChunkEntities(ChunkEntities&&) = delete;
			~ChunkEntities();

			void Update();

			ChunkEntities& operator=(const ChunkEntities&) = delete;
			ChunkEntities& operator=(ChunkEntities&&) = delete;

		protected:
			struct NoInit {};
			ChunkEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, NoInit);

			void CreateChunkEntity(std::size_t chunkId, const Chunk* chunk);
			void DestroyChunkEntity(std::size_t chunkId);
			void FillChunks();
			virtual void HandleChunkUpdate(std::size_t chunkId, const Chunk* chunk);
			void UpdateChunkEntity(std::size_t chunkId);

			struct UpdateJob
			{
				std::function<void(std::size_t chunkId, UpdateJob&& job)> applyFunc;
				std::atomic_bool cancelled = false;
				std::atomic_uint executionCounter = 0;
				unsigned int taskCount;
			};

			struct ColliderUpdateJob : UpdateJob
			{
				std::shared_ptr<Nz::Collider3D> collider;
			};

			NazaraSlot(ChunkContainer, OnChunkAdded, m_onChunkAdded);
			NazaraSlot(ChunkContainer, OnChunkRemove, m_onChunkRemove);
			NazaraSlot(ChunkContainer, OnChunkUpdated, m_onChunkUpdated);

			std::vector<entt::handle> m_chunkEntities;
			tsl::hopscotch_map<std::size_t /*chunkId*/, std::shared_ptr<UpdateJob>> m_updateJobs;
			Nz::Bitset<> m_invalidatedChunks;
			Nz::ApplicationBase& m_application;
			Nz::EnttWorld& m_world;
			const BlockLibrary& m_blockLibrary;
			const ChunkContainer& m_chunkContainer;
	};
}

#include <CommonLib/ChunkEntities.inl>

#endif // TSOM_COMMONLIB_CHUNKENTITIES_HPP
