// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP
#define TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP

#include <CommonLib/Chunk.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <ServerLib/Export.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>

namespace tsom
{
	class NetworkSession;

	class TSOM_SERVERLIB_API SessionVisibilityHandler
	{
		public:
			struct CreateEntityData;

			inline SessionVisibilityHandler(NetworkSession* networkSession);
			SessionVisibilityHandler(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler(SessionVisibilityHandler&&) = delete;
			~SessionVisibilityHandler() = default;

			void CreateChunk(Chunk& chunk);
			void CreateEntity(entt::handle entity, CreateEntityData entityData);

			void DestroyChunk(Chunk& chunk);
			void DestroyEntity(entt::handle entity);

			void Dispatch(Nz::UInt16 tickIndex);

			Chunk* GetChunkByIndex(std::size_t chunkIndex) const;

			SessionVisibilityHandler& operator=(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler& operator=(SessionVisibilityHandler&&) = delete;

			struct CreateEntityData
			{
				Nz::Quaternionf initialRotation;
				Nz::Vector3f initialPosition;
				std::optional<Packets::Helper::PlayerControlledData> playerControlledData;
				bool isMoving;
			};

		private:
			void DispatchChunks();
			void DispatchEntities(Nz::UInt16 tickIndex);

			static constexpr std::size_t FreeChunkIdGrowRate = 128;
			static constexpr std::size_t FreeEntityIdGrowRate = 512;

			struct HandlerHasher
			{
				inline std::size_t operator()(const entt::handle& handle) const;
			};

			struct VisibleChunk
			{
				NazaraSlot(Chunk, OnBlockUpdated, onCellUpdatedSlot);

				Chunk* chunk;
				Packets::ChunkUpdate chunkUpdatePacket;
			};

			tsl::hopscotch_map<entt::handle, Nz::UInt32, HandlerHasher> m_entityToNetworkId;
			tsl::hopscotch_map<entt::handle, CreateEntityData, HandlerHasher> m_createdEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_deletedEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_movingEntities;
			tsl::hopscotch_map<Chunk*, std::size_t> m_chunkIndices;
			std::vector<VisibleChunk> m_visibleChunks;
			Nz::Bitset<Nz::UInt64> m_freeChunkIds;
			Nz::Bitset<Nz::UInt64> m_freeEntityIds;
			Nz::Bitset<Nz::UInt64> m_newlyHiddenChunk;
			Nz::Bitset<Nz::UInt64> m_newlyVisibleChunk;
			Nz::Bitset<Nz::UInt64> m_updatedChunk;
			NetworkSession* m_networkSession;
	};
}

#include <ServerLib/SessionVisibilityHandler.inl>

#endif
