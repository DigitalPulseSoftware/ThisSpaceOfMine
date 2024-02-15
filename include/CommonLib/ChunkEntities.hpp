// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CHUNKENTITIES_HPP
#define TSOM_COMMONLIB_CHUNKENTITIES_HPP

#include <CommonLib/ChunkContainer.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace Nz
{
	class EnttWorld;
}

namespace tsom
{
	class BlockLibrary;
	class Chunk;

	class TSOM_COMMONLIB_API ChunkEntities
	{
		public:
			ChunkEntities(Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary);
			ChunkEntities(const ChunkEntities&) = delete;
			ChunkEntities(ChunkEntities&&) = delete;
			~ChunkEntities();

			inline bool DoesRequireUpdate() const;

			void Update();

			ChunkEntities& operator=(const ChunkEntities&) = delete;
			ChunkEntities& operator=(ChunkEntities&&) = delete;

		protected:
			struct NoInit {};
			ChunkEntities(Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, NoInit);

			virtual void CreateChunkEntity(std::size_t chunkId, const Chunk* chunk);
			void DestroyChunkEntity(std::size_t chunkId);
			void FillChunks();
			virtual void UpdateChunkEntity(std::size_t chunkId);

			NazaraSlot(ChunkContainer, OnChunkAdded, m_onChunkAdded);
			NazaraSlot(ChunkContainer, OnChunkRemove, m_onChunkRemove);
			NazaraSlot(ChunkContainer, OnChunkUpdated, m_onChunkUpdated);

			std::vector<entt::handle> m_chunkEntities;
			Nz::EnttWorld& m_world;
			const BlockLibrary& m_blockLibrary;
			const ChunkContainer& m_chunkContainer;
			Nz::Bitset<> m_invalidatedChunks;
	};
}

#include <CommonLib/ChunkEntities.inl>

#endif // TSOM_COMMONLIB_CHUNKENTITIES_HPP
