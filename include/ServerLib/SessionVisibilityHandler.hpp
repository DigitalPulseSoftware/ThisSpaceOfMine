// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
			bool CreateEnvironment(ServerEnvironment& environment, entt::handle environmentOwner = {});

			void DestroyChunk(entt::handle entity, Chunk& chunk);
			void DestroyEntity(entt::handle entity);
			void DestroyEnvironment(ServerEnvironment& environment);

			void Dispatch(Nz::UInt16 tickIndex);

			inline bool GetChunkByNetworkId(Packets::Helper::ChunkId networkId, entt::handle* entityOwner, Chunk** chunk) const;
			inline bool GetEntityByNetworkId(Packets::Helper::EntityId networkId, entt::handle* entity) const;
			inline Packets::Helper::EnvironmentId GetEnvironmentId(ServerEnvironment* environment) const;

			inline void TriggerEntityRpc(entt::handle entity, Nz::UInt32 rpcIndex);

			inline void UpdateControlledEntity(entt::handle entity, CharacterController* controller);
			inline void UpdateEntityProperty(entt::handle entity, Nz::UInt32 propertyIndex, const EntityProperty& newValue);
			void UpdateEntityEnvironment(ServerEnvironment& newEnvironment, entt::handle oldEntity, entt::handle newEntity);
			inline void UpdateLastInputIndex(InputIndex inputIndex);

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

			void DispatchEntitiesCreation(Nz::UInt16 tickIndex);
			void DispatchEntitiesDeletion(Nz::UInt16 tickIndex);
			void DispatchEntitiesEnvironmentUpdate(Nz::UInt16 tickIndex);
			void DispatchEntitiesProperties(Nz::UInt16 tickIndex);
			void DispatchEntitiesRpcs(Nz::UInt16 tickIndex);
			void DispatchEntitiesStates(Nz::UInt16 tickIndex);

			void DispatchEnvironments(Nz::UInt16 tickIndex);
			void HandleEntityCreation(std::vector<Packets::Helper::EntityData>& entities, entt::handle entity, CreateEntityData&& createEntityData);
			void HandleEntityDestruction(entt::handle entity);

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

			struct EntityPropertyData
			{
				Nz::UInt32 propertiesMask;
				Nz::HybridVector<EntityProperty, 3> values;
			};

			struct EnvironmentData
			{
				ServerEnvironment* environment = nullptr;
				entt::handle owner;
				Nz::Bitset<Nz::UInt64> entities;
			};

			struct EnvironmentCreationData
			{
				ServerEnvironment* environment;
				entt::handle owner;
				tsl::hopscotch_map<entt::handle, CreateEntityData, HandlerHasher> createdEntities;
			};

			struct EnvironmentOwnerUpdate
			{
				ServerEnvironment* environment;
				entt::handle newOwner;
			};

			struct EnvironmentUpdate
			{
				entt::handle newEntity;
				ServerEnvironment* oldEnvironment;
				ServerEnvironment* newEnvironment;
			};

			using ChunkNetworkMap = tsl::hopscotch_map<ChunkIndices, ChunkId>;

			tsl::hopscotch_map<entt::handle, EntityId, HandlerHasher> m_entityIndices;
			tsl::hopscotch_map<entt::handle, CreateEntityData, HandlerHasher> m_createdEntities;
			tsl::hopscotch_map<entt::handle, EntityPropertyData, HandlerHasher> m_propertyUpdatedEntities;
			tsl::hopscotch_map<entt::handle, Nz::HybridVector<Nz::UInt32, 3>, HandlerHasher> m_triggeredEntitiesRpc;
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
			std::vector<EnvironmentCreationData> m_createdEnvironments;
			std::vector<EnvironmentOwnerUpdate> m_environmentOwnerUpdates;
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
	};
}

#include <ServerLib/SessionVisibilityHandler.inl>

#endif // TSOM_SERVERLIB_SESSIONVISIBILITYHANDLER_HPP
