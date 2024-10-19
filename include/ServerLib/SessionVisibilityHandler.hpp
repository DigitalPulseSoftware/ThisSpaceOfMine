// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP
#define TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/EnvironmentTransform.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/Node.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>
#include <memory>

namespace tsom
{
	class CharacterController;
	class EntityClass;
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

			bool CreateChunk(entt::handle entity, Chunk& chunk);
			void CreateEntity(entt::handle entity, CreateEntityData entityData);
			bool CreateEnvironment(ServerEnvironment& environment, const EnvironmentTransform& transform);

			void DestroyChunk(entt::handle entity, Chunk& chunk);
			void DestroyEntity(entt::handle entity);
			void DestroyEnvironment(ServerEnvironment& environment);

			void Dispatch(Nz::UInt16 tickIndex);

			inline bool GetChunkByNetworkId(Packets::Helper::ChunkId networkId, entt::handle* entityOwner, Chunk** chunk) const;
			inline bool GetEntityByNetworkId(Packets::Helper::EntityId networkId, entt::handle* entity) const;
			inline Packets::Helper::EnvironmentId GetEnvironmentId(ServerEnvironment* environment) const;

			inline void MoveEnvironment(ServerEnvironment& environment, const EnvironmentTransform& transform);

			inline void TriggerEntityRpc(entt::handle entity, Nz::UInt32 rpcIndex);

			inline void UpdateControlledEntity(entt::handle entity, CharacterController* controller);
			inline void UpdateEntityProperty(entt::handle entity, Nz::UInt32 propertyIndex);
			void UpdateEntityEnvironment(ServerEnvironment& newEnvironment, entt::handle oldEntity, entt::handle newEntity);
			inline void UpdateLastInputIndex(InputIndex inputIndex);
			inline void UpdateRootEnvironment(ServerEnvironment& environment);

			SessionVisibilityHandler& operator=(const SessionVisibilityHandler&) = delete;
			SessionVisibilityHandler& operator=(SessionVisibilityHandler&&) = delete;

			struct CreateEntityData
			{
				const ServerEnvironment* environment;
				std::shared_ptr<const EntityClass> entityClass;
				Nz::Quaternionf initialRotation;
				Nz::Vector3f initialPosition;
				std::optional<Packets::Helper::PlayerControlledData> playerControlledData;
				std::vector<EntityProperty> entityProperties;
				bool isMoving;
			};

		private:
			void DispatchChunks(Nz::UInt16 tickIndex);
			void DispatchChunkCreation(Nz::UInt16 tickIndex);
			void DispatchChunkReset(Nz::UInt16 tickIndex);
			void DispatchEntities(Nz::UInt16 tickIndex);
			void DispatchEnvironments(Nz::UInt16 tickIndex);
			void HandleEntityDestruction(entt::handle entity);

			static constexpr std::size_t MaxConcurrentChunkUpdate = 3;
			static constexpr std::size_t FreeChunkIdGrowRate = 128;
			static constexpr std::size_t FreeEntityIdGrowRate = 512;
			static constexpr std::size_t FreeNetworkIdGrowRate = 64;

			using ChunkId = Packets::Helper::ChunkId;
			using EntityId = Packets::Helper::EntityId;
			using EnvironmentId = Packets::Helper::EnvironmentId;

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
				ServerEnvironment* environment;
				Nz::Bitset<Nz::UInt64> entities;
			};

			struct EnvironmentTransformation
			{
				ServerEnvironment* environment;
				EnvironmentTransform transform;
			};

			struct EnvironmentUpdate
			{
				entt::handle newEntity;
				ServerEnvironment* oldEnvironment;
				ServerEnvironment* newEnvironment;
			};

			struct HandlerHasher
			{
				inline std::size_t operator()(const entt::handle& handle) const;
			};

			using ChunkNetworkMap = tsl::hopscotch_map<ChunkIndices, ChunkId>;

			tsl::hopscotch_map<entt::handle, EntityId, HandlerHasher> m_entityIndices;
			tsl::hopscotch_map<entt::handle, CreateEntityData, HandlerHasher> m_createdEntities;
			tsl::hopscotch_map<entt::handle, Nz::UInt32, HandlerHasher> m_propertyUpdatedEntities;
			tsl::hopscotch_map<entt::handle, std::vector<Nz::UInt32>, HandlerHasher> m_triggeredEntitiesRpc;
			tsl::hopscotch_map<entt::handle, ChunkNetworkMap, HandlerHasher> m_chunkNetworkMaps;
			tsl::hopscotch_map<const ServerEnvironment*, EnvironmentId> m_environmentIndices;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_deletedEntities;
			tsl::hopscotch_set<entt::handle, HandlerHasher> m_movingEntities;
			std::shared_ptr<std::size_t> m_activeChunkUpdates;
			std::vector<ServerEnvironment*> m_destroyedEnvironments;
			std::vector<ChunkData> m_visibleChunks;
			std::vector<ChunkWithPos> m_orderedChunkList;
			std::vector<EntityData> m_visibleEntities;
			std::vector<EnvironmentData> m_visibleEnvironments;
			std::vector<EnvironmentTransformation> m_createdEnvironments;
			std::vector<EnvironmentTransformation> m_environmentTransformations;
			std::vector<EnvironmentUpdate> m_environmentUpdates;
			Nz::Bitset<Nz::UInt64> m_freeChunkIds;
			Nz::Bitset<Nz::UInt64> m_freeEntityIds;
			Nz::Bitset<Nz::UInt64> m_freeEnvironmentIds;
			Nz::Bitset<Nz::UInt64> m_newlyHiddenChunk;
			Nz::Bitset<Nz::UInt64> m_newlyVisibleChunk;
			Nz::Bitset<Nz::UInt64> m_resetChunk;
			Nz::Bitset<Nz::UInt64> m_updatedChunk;
			entt::handle m_controlledEntity;
			EnvironmentId m_currentEnvironmentId;
			InputIndex m_lastInputIndex;
			CharacterController* m_controlledCharacter;
			NetworkSession* m_networkSession;
			ServerEnvironment* m_nextRootEnvironment;
	};
}

#include <ServerLib/SessionVisibilityHandler.inl>

#endif // TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP
