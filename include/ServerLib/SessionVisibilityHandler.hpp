// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP
#define TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>
#include <memory>

namespace tsom
{
	class CharacterController;
	class NetworkSession;

	class TSOM_SERVERLIB_API SessionVisibilityHandler
	{
		public:
			struct CreateEntityData;

			inline SessionVisibilityHandler(NetworkSession* networkSession);
			SessionVisibilityHandler(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler(SessionVisibilityHandler&&) = delete;
			~SessionVisibilityHandler() = default;

			void CreateChunk(const Chunk& chunk);
			void CreateEntity(entt::handle entity, CreateEntityData entityData);

			void DestroyChunk(const Chunk& chunk);
			void DestroyEntity(entt::handle entity);

			void Dispatch(Nz::UInt16 tickIndex);

			const Chunk* GetChunkByIndex(std::size_t chunkIndex) const;

			inline void UpdateControlledEntity(entt::handle entity, CharacterController* controller);
			inline void UpdateLastInputIndex(InputIndex inputIndex);

			SessionVisibilityHandler& operator=(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler& operator=(SessionVisibilityHandler&&) = delete;

			struct CreateEntityData
			{
				Nz::Quaternionf initialRotation;
				Nz::Vector3f initialPosition;
				std::optional<Packets::Helper::PlayerControlledData> playerControlledData;
				std::optional<Packets::Helper::ShipData> shipData;
				bool isMoving;
			};

		private:
			void DispatchChunks();
			void DispatchChunkCreation();
			void DispatchChunkReset();
			void DispatchEntities(Nz::UInt16 tickIndex);

			static constexpr std::size_t MaxConcurrentChunkUpdate = 3;
			static constexpr std::size_t FreeChunkIdGrowRate = 128;
			static constexpr std::size_t FreeEntityIdGrowRate = 512;

			struct ChunkWithPos
			{
				std::size_t chunkIndex;
				Nz::Vector3f chunkCenter;
			};

			struct HandlerHasher
			{
				inline std::size_t operator()(const entt::handle& handle) const;
			};

			struct VisibleChunk
			{
				NazaraSlot(Chunk, OnBlockUpdated, onBlockUpdatedSlot);
				NazaraSlot(Chunk, OnReset, onResetSlot);

				const Chunk* chunk;
				Packets::ChunkUpdate chunkUpdatePacket;
			};

			tsl::hopscotch_map<entt::handle, Nz::UInt32, HandlerHasher> m_entityToNetworkId;
			tsl::hopscotch_map<entt::handle, CreateEntityData, HandlerHasher> m_createdEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_deletedEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_movingEntities;
			tsl::hopscotch_map<const Chunk*, std::size_t> m_chunkIndices;
			std::shared_ptr<std::size_t> m_activeChunkUpdates;
			std::vector<ChunkWithPos> m_orderedChunkList;
			std::vector<VisibleChunk> m_visibleChunks;
			Nz::Bitset<Nz::UInt64> m_freeChunkIds;
			Nz::Bitset<Nz::UInt64> m_freeEntityIds;
			Nz::Bitset<Nz::UInt64> m_newlyHiddenChunk;
			Nz::Bitset<Nz::UInt64> m_newlyVisibleChunk;
			Nz::Bitset<Nz::UInt64> m_resetChunk;
			Nz::Bitset<Nz::UInt64> m_updatedChunk;
			entt::handle m_controlledEntity;
			InputIndex m_lastInputIndex;
			CharacterController* m_controlledCharacter;
			NetworkSession* m_networkSession;
	};
}

#include <ServerLib/SessionVisibilityHandler.inl>

#endif // TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP
