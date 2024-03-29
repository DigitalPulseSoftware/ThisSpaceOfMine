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
	class ServerEnvironment;

	class TSOM_SERVERLIB_API SessionVisibilityHandler
	{
		public:
			struct CreateEntityData;

			inline SessionVisibilityHandler(NetworkSession* networkSession);
			SessionVisibilityHandler(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler(SessionVisibilityHandler&&) = delete;
			~SessionVisibilityHandler() = default;

			void CreateChunk(entt::handle entity, Chunk& chunk);
			void CreateEntity(entt::handle entity, CreateEntityData entityData);
			void CreateEnvironment(ServerEnvironment& environment);

			void DestroyChunk(Chunk& chunk);
			void DestroyEntity(entt::handle entity);
			void DestroyEnvironment(ServerEnvironment& environment);

			void Dispatch(Nz::UInt16 tickIndex);

			Chunk* GetChunkByIndex(std::size_t chunkIndex) const;

			inline void UpdateControlledEntity(entt::handle entity, CharacterController* controller);
			inline void UpdateLastInputIndex(InputIndex inputIndex);

			SessionVisibilityHandler& operator=(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler& operator=(SessionVisibilityHandler&&) = delete;

			struct CreateEntityData
			{
				ServerEnvironment* environment;
				Nz::Quaternionf initialRotation;
				Nz::Vector3f initialPosition;
				std::optional<Packets::Helper::PlanetData> planetData;
				std::optional<Packets::Helper::PlayerControlledData> playerControlledData;
				std::optional<Packets::Helper::ShipData> shipData;
				bool isMoving;
			};

		private:
			void DispatchChunks(Nz::UInt16 tickIndex);
			void DispatchChunkCreation(Nz::UInt16 tickIndex);
			void DispatchChunkReset(Nz::UInt16 tickIndex);
			void DispatchEntities(Nz::UInt16 tickIndex);
			void DispatchEnvironments(Nz::UInt16 tickIndex);

			static constexpr std::size_t MaxConcurrentChunkUpdate = 3;
			static constexpr std::size_t FreeChunkIdGrowRate = 128;
			static constexpr std::size_t FreeEntityIdGrowRate = 512;
			static constexpr std::size_t FreeNetworkIdGrowRate = 64;

			using ChunkId = Packets::Helper::ChunkId;
			using EntityId = Packets::Helper::EntityId;
			using EnvironmentId = Packets::Helper::EnvironmentId;

			struct HandlerHasher
			{
				inline std::size_t operator()(const entt::handle& handle) const;
			};

			struct ChunkData
			{
				NazaraSlot(Chunk, OnBlockUpdated, onBlockUpdatedSlot);
				NazaraSlot(Chunk, OnReset, onResetSlot);

				entt::handle entityOwner;
				Chunk* chunk;
				Packets::ChunkUpdate chunkUpdatePacket;
			};

			struct ChunkWithPos
			{
				std::size_t chunkIndex;
				Nz::Vector3f chunkCenter;
			};

			struct EntityData
			{
				entt::handle entity;
				EnvironmentId envIndex;
			};

			struct EnvironmentData
			{
				Nz::Bitset<Nz::UInt64> chunks;
				Nz::Bitset<Nz::UInt64> entities;
			};

			tsl::hopscotch_map<entt::handle, EntityId, HandlerHasher> m_entityIndices;
			tsl::hopscotch_map<entt::handle, CreateEntityData, HandlerHasher> m_createdEntities;
			tsl::hopscotch_map<Chunk*, ChunkId> m_chunkIndices;
			tsl::hopscotch_map<ServerEnvironment*, EnvironmentId> m_environmentIndices;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_deletedEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_movingEntities;
			std::shared_ptr<std::size_t> m_activeChunkUpdates;
			std::vector<ServerEnvironment*> m_createdEnvironments;
			std::vector<ServerEnvironment*> m_destroyedEnvironments;
			std::vector<ChunkData> m_visibleChunks;
			std::vector<ChunkWithPos> m_orderedChunkList;
			std::vector<EntityData> m_visibleEntities;
			std::vector<EnvironmentData> m_visibleEnvironments;
			Nz::Bitset<Nz::UInt64> m_freeChunkIds;
			Nz::Bitset<Nz::UInt64> m_freeEntityIds;
			Nz::Bitset<Nz::UInt64> m_freeEnvironmentIds;
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
