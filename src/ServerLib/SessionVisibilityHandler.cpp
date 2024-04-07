// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/SessionVisibilityHandler.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	bool SessionVisibilityHandler::CreateChunk(entt::handle entity, Chunk& chunk)
	{
		// Check if this chunk was marked for destruction
		if (auto it = m_chunkIndices.find(&chunk); it != m_chunkIndices.end())
		{
			// Chunk still exists, resurrect it
			m_newlyHiddenChunk.Reset(it->second);
			return false;
		}
		else
		{
			// Client is not aware of this chunk
			std::size_t chunkIndex = m_freeChunkIds.FindFirst();
			if (chunkIndex == m_freeChunkIds.npos)
			{
				chunkIndex = m_freeChunkIds.GetSize();
				m_freeChunkIds.Resize(chunkIndex + FreeChunkIdGrowRate, true);
			}

			m_freeChunkIds.Set(chunkIndex, false);
			m_newlyVisibleChunk.UnboundedSet(chunkIndex);

			assert(!m_chunkIndices.contains(&chunk));
			m_chunkIndices.emplace(&chunk, Nz::SafeCaster(chunkIndex));

			if (chunkIndex >= m_visibleChunks.size())
				m_visibleChunks.resize(chunkIndex + 1);

			ChunkData& chunkData = m_visibleChunks[chunkIndex];
			chunkData.chunk = &chunk;
			chunkData.chunkUpdatePacket.chunkId = Nz::SafeCast<ChunkId>(chunkIndex);
			chunkData.entityOwner = entity;

			return true;
		}
	}

	void SessionVisibilityHandler::CreateEntity(entt::handle entity, CreateEntityData entityData)
	{
		if (entityData.isMoving && entity != m_controlledEntity)
			m_movingEntities.emplace(entity);

		m_createdEntities.emplace(entity, std::move(entityData));
	}

	bool SessionVisibilityHandler::CreateEnvironment(ServerEnvironment& environment, const EnvironmentTransform& transform)
	{
		if (auto it = std::find(m_destroyedEnvironments.begin(), m_destroyedEnvironments.end(), &environment); it != m_destroyedEnvironments.end())
		{
			m_destroyedEnvironments.erase(it);
			MoveEnvironment(environment, transform);
			return false;
		}
		else
		{
			assert(std::find_if(m_createdEnvironments.begin(), m_createdEnvironments.end(), [&](const EnvironmentTransformation& transform) { return transform.environment == &environment; }) == m_createdEnvironments.end());
			m_createdEnvironments.push_back({
				.environment = &environment,
				.transform = transform
			});

			return true;
		}
	}

	void SessionVisibilityHandler::DestroyChunk(Chunk& chunk)
	{
		std::size_t chunkIndex = Nz::Retrieve(m_chunkIndices, &chunk);

		// Is this a newly visible chunk not sent to the client?
		if (m_newlyVisibleChunk.Test(chunkIndex))
			m_newlyVisibleChunk.Reset(chunkIndex); //< dismiss it

		m_newlyHiddenChunk.UnboundedSet(chunkIndex);
	}

	void SessionVisibilityHandler::DestroyEntity(entt::handle entity)
	{
		assert(!m_deletedEntities.contains(entity));
		m_createdEntities.erase(entity);
		m_movingEntities.erase(entity);
		m_deletedEntities.emplace(entity);
	}

	void SessionVisibilityHandler::DestroyEnvironment(ServerEnvironment& environment)
	{
		if (auto it = std::find_if(m_createdEnvironments.begin(), m_createdEnvironments.end(), [&](const EnvironmentTransformation& transform) { return transform.environment == &environment; }); it != m_createdEnvironments.end())
			m_createdEnvironments.erase(it);
		else
		{
			assert(std::find(m_destroyedEnvironments.begin(), m_destroyedEnvironments.end(), &environment) == m_destroyedEnvironments.end());
			m_destroyedEnvironments.push_back(&environment);
		}

		if (auto it = std::find_if(m_environmentTransformations.begin(), m_environmentTransformations.end(), [&](const EnvironmentTransformation& transform) { return transform.environment == &environment; }); it != m_environmentTransformations.end())
			m_environmentTransformations.erase(it);
	}

	void SessionVisibilityHandler::Dispatch(Nz::UInt16 tickIndex)
	{
		DispatchEnvironments(tickIndex);
		DispatchEntities(tickIndex);
		DispatchChunks(tickIndex);
	}

	void SessionVisibilityHandler::UpdateEntityEnvironment(ServerEnvironment& newEnvironment, entt::handle oldEntity, entt::handle newEntity)
	{
		// Don't remove from created entities as client will need it to update its environment
		m_deletedEntities.erase(oldEntity);
		if (m_movingEntities.erase(oldEntity) > 0)
			m_movingEntities.insert(newEntity);

		auto it = m_entityIndices.find(oldEntity);
		EntityId entityIndex = it.value();
		m_entityIndices.erase(it);
		m_entityIndices.emplace(newEntity, it.value());

		m_environmentUpdates.push_back({ entityIndex, &newEnvironment });
	}

	void SessionVisibilityHandler::DispatchChunks(Nz::UInt16 tickIndex)
	{
		for (std::size_t chunkIndex : m_newlyHiddenChunk.IterBits())
		{
			ChunkData& visibleChunk = m_visibleChunks[chunkIndex];

			EntityId entityIndex = Nz::Retrieve(m_entityIndices, visibleChunk.entityOwner);
			EnvironmentId envIndex = m_visibleEntities[entityIndex].envIndex;
			m_visibleEnvironments[envIndex].chunks.Reset(chunkIndex);

			// Mark chunk index as free on dispatch to prevent chunk index reuse while resurrecting it
			m_freeChunkIds.Set(chunkIndex);
			m_resetChunk.UnboundedReset(chunkIndex);
			m_updatedChunk.UnboundedReset(chunkIndex);

			Packets::ChunkDestroy chunkDestroyPacket;
			chunkDestroyPacket.chunkId = Nz::SafeCast<ChunkId>(chunkIndex);
			chunkDestroyPacket.entityId = entityIndex;
			chunkDestroyPacket.tickIndex = tickIndex;

			m_networkSession->SendPacket(chunkDestroyPacket);

			visibleChunk.chunk = nullptr;
			visibleChunk.entityOwner = entt::handle{};
			visibleChunk.onBlockUpdatedSlot.Disconnect();
		}
		m_newlyHiddenChunk.Clear();

		if (m_newlyVisibleChunk.GetSize() > 0)
			DispatchChunkCreation(tickIndex);

		if (m_resetChunk.GetSize() > 0)
			DispatchChunkReset(tickIndex);

		for (std::size_t chunkIndex : m_updatedChunk.IterBits())
		{
			ChunkData& visibleChunk = m_visibleChunks[chunkIndex];
			m_networkSession->SendPacket(visibleChunk.chunkUpdatePacket);
			visibleChunk.chunkUpdatePacket.updates.clear();
		}
		m_updatedChunk.Clear();
	}

	void SessionVisibilityHandler::DispatchChunkCreation(Nz::UInt16 tickIndex)
	{
		for (std::size_t chunkIndex : m_newlyVisibleChunk.IterBits())
		{
			ChunkData& visibleChunk = m_visibleChunks[chunkIndex];

			// Connect update signal on dispatch to prevent updates made during the same tick to be sent as update
			visibleChunk.chunkUpdatePacket.entityId = Nz::Retrieve(m_entityIndices, visibleChunk.entityOwner);

			visibleChunk.onBlockUpdatedSlot.Connect(visibleChunk.chunk->OnBlockUpdated, [this, chunkIndex]([[maybe_unused]] Chunk* chunk, const Nz::Vector3ui& indices, BlockIndex newBlock)
			{
				m_updatedChunk.UnboundedSet(chunkIndex);

				ChunkData& visibleChunk = m_visibleChunks[chunkIndex];
				assert(visibleChunk.chunk == chunk);

				// Chunk content has been reset or wasn't already sent
				if (m_resetChunk.UnboundedTest(chunkIndex))
					return;

				auto comp = [](Packets::ChunkUpdate::BlockUpdate& blockUpdate, const Nz::Vector3ui& indices)
				{
					return Nz::Vector3ui(blockUpdate.voxelLoc.x, blockUpdate.voxelLoc.y, blockUpdate.voxelLoc.z) < indices;
				};

				auto it = std::lower_bound(visibleChunk.chunkUpdatePacket.updates.begin(), visibleChunk.chunkUpdatePacket.updates.end(), indices, comp);
				if (it == visibleChunk.chunkUpdatePacket.updates.end() || Nz::Vector3ui(it->voxelLoc.x, it->voxelLoc.y, it->voxelLoc.z) != indices)
				{
					visibleChunk.chunkUpdatePacket.updates.insert(it, {
						Packets::Helper::VoxelLocation{ Nz::SafeCast<Nz::UInt8>(indices.x), Nz::SafeCast<Nz::UInt8>(indices.y), Nz::SafeCast<Nz::UInt8>(indices.z) },
						Nz::SafeCast<Nz::UInt8>(newBlock)
					});
				}
				else
					it->newContent = Nz::SafeCast<Nz::UInt8>(newBlock);
			});

			visibleChunk.onResetSlot.Connect(visibleChunk.chunk->OnReset, [this, chunkIndex](Chunk*)
			{
				m_resetChunk.UnboundedSet(chunkIndex);
			});

			// Register chunk to environment
			EntityId entityIndex = Nz::Retrieve(m_entityIndices, visibleChunk.entityOwner);
			EnvironmentId envIndex = m_visibleEntities[entityIndex].envIndex;
			m_visibleEnvironments[envIndex].chunks.UnboundedSet(chunkIndex);

			ChunkIndices chunkLocation = visibleChunk.chunk->GetIndices();
			Nz::Vector3ui chunkSize = visibleChunk.chunk->GetSize();

			Packets::ChunkCreate chunkCreatePacket;
			chunkCreatePacket.chunkId = Nz::SafeCast<ChunkId>(chunkIndex);
			chunkCreatePacket.chunkLocX = chunkLocation.x;
			chunkCreatePacket.chunkLocY = chunkLocation.y;
			chunkCreatePacket.chunkLocZ = chunkLocation.z;
			chunkCreatePacket.chunkSizeX = chunkSize.x;
			chunkCreatePacket.chunkSizeY = chunkSize.y;
			chunkCreatePacket.chunkSizeZ = chunkSize.z;
			chunkCreatePacket.cellSize = visibleChunk.chunk->GetBlockSize();
			chunkCreatePacket.entityId = entityIndex;
			chunkCreatePacket.tickIndex = tickIndex;

			m_networkSession->SendPacket(chunkCreatePacket);

			m_newlyVisibleChunk.UnboundedReset(chunkIndex);
			m_resetChunk.UnboundedSet(chunkIndex);
		}
		m_newlyVisibleChunk.Clear();
	}

	void SessionVisibilityHandler::DispatchChunkReset(Nz::UInt16 tickIndex)
	{
		m_orderedChunkList.clear();
		for (std::size_t chunkIndex : m_resetChunk.IterBits())
		{
			const Chunk* chunk = m_visibleChunks[chunkIndex].chunk;
			Nz::Vector3f chunkPosition = chunk->GetContainer().GetChunkOffset(chunk->GetIndices());
			m_orderedChunkList.push_back(ChunkWithPos{ chunkIndex, chunkPosition + Nz::Vector3f(chunk->GetSize()) * chunk->GetBlockSize() });
		}

		if (m_controlledEntity)
		{
			// Sort chunks based on distance to reference position (closers chunks get sent in priority)
			Nz::Vector3f referencePosition = m_controlledEntity.get<Nz::NodeComponent>().GetGlobalPosition();

			std::sort(m_orderedChunkList.begin(), m_orderedChunkList.end(), [&](const ChunkWithPos& chunkA, const ChunkWithPos& chunkB)
			{
				return chunkA.chunkCenter.SquaredDistance(referencePosition) < chunkB.chunkCenter.SquaredDistance(referencePosition);
			});
		}

		for (const ChunkWithPos& chunk : m_orderedChunkList)
		{
			if (*m_activeChunkUpdates >= MaxConcurrentChunkUpdate)
				return;

			ChunkData& visibleChunk = m_visibleChunks[chunk.chunkIndex];

			ChunkIndices chunkLocation = visibleChunk.chunk->GetIndices();
			Nz::Vector3ui chunkSize = visibleChunk.chunk->GetSize();

			Packets::ChunkReset chunkResetPacket;
			chunkResetPacket.chunkId = Nz::SafeCast<ChunkId>(chunk.chunkIndex);
			chunkResetPacket.entityId = Nz::Retrieve(m_entityIndices, visibleChunk.entityOwner);
			chunkResetPacket.tickIndex = tickIndex;

			unsigned int blockCount = chunkSize.x * chunkSize.y * chunkSize.z;
			chunkResetPacket.content.resize(blockCount);

			const BlockIndex* chunkContent = visibleChunk.chunk->GetContent();
			std::memcpy(chunkResetPacket.content.data(), chunkContent, blockCount * sizeof(BlockIndex));

			(*m_activeChunkUpdates)++;
			m_networkSession->SendPacket(chunkResetPacket, [chunkLocation, chunkUpdateCount = m_activeChunkUpdates]
			{
				assert(*chunkUpdateCount > 0);
				(*chunkUpdateCount)--;
			});

			m_resetChunk.UnboundedReset(chunk.chunkIndex);
		}

		// If we get there, we didn't hit the concurrent chunk limit, we can clear the chunk bitset
		assert(m_resetChunk.TestNone());
		m_resetChunk.Clear();
	}

	void SessionVisibilityHandler::DispatchEntities(Nz::UInt16 tickIndex)
	{
		if (!m_deletedEntities.empty())
		{
			Packets::EntitiesDelete deletePacket;
			deletePacket.tickIndex = tickIndex;

			for (const entt::handle& handle : m_deletedEntities)
			{
				Nz::UInt32 entityId = Nz::Retrieve(m_entityIndices, handle);
				deletePacket.entities.push_back(entityId);

				m_freeEntityIds.Set(entityId, true);
				m_visibleEntities[entityId].entity = entt::handle{};
				m_visibleEntities[entityId].envIndex = std::numeric_limits<EnvironmentId>::max();

				m_entityIndices.erase(handle);

				auto it = std::find_if(m_environmentUpdates.begin(), m_environmentUpdates.end(), [&](const EnvironmentUpdate& environmentUpdate)
				{
					return environmentUpdate.entityId == entityId;
				});
				if (it != m_environmentUpdates.end())
					m_environmentUpdates.erase(it);
			}

			m_networkSession->SendPacket(deletePacket);
			m_deletedEntities.clear();
		}

		if (!m_createdEntities.empty())
		{
			Packets::EntitiesCreation creationPacket;
			creationPacket.tickIndex = tickIndex;

			for (auto it = m_createdEntities.begin(); it != m_createdEntities.end(); ++it)
			{
				entt::handle handle = it.key();
				auto& data = it.value();

				std::size_t entityIndex = m_freeEntityIds.FindFirst();
				if (entityIndex == m_freeEntityIds.npos)
				{
					entityIndex = m_freeEntityIds.GetSize();
					m_freeEntityIds.Resize(entityIndex + FreeEntityIdGrowRate, true);
				}

				m_freeEntityIds.Set(entityIndex, false);

				if (entityIndex >= m_visibleEntities.size())
					m_visibleEntities.resize(entityIndex + 1);

				EnvironmentId envIndex = Nz::Retrieve(m_environmentIndices, data.environment);

				m_visibleEntities[entityIndex].entity = handle;
				m_visibleEntities[entityIndex].envIndex = envIndex;
				m_visibleEnvironments[envIndex].entities.UnboundedSet(entityIndex);

				m_entityIndices[handle] = entityIndex;

				auto& entityData = creationPacket.entities.emplace_back();
				entityData.entityId = Nz::SafeCast<EntityId>(entityIndex);
				entityData.environmentId = envIndex;
				entityData.initialStates.position = data.initialPosition;
				entityData.initialStates.rotation = data.initialRotation;
				entityData.planet = std::move(data.planetData);
				entityData.playerControlled = std::move(data.playerControlledData);
				entityData.ship = std::move(data.shipData);
			}

			m_networkSession->SendPacket(creationPacket);
			m_createdEntities.clear();
		}

		if (!m_environmentUpdates.empty())
		{
			for (const EnvironmentUpdate& envUpdate : m_environmentUpdates)
			{
				EnvironmentId envIndex = Nz::Retrieve(m_environmentIndices, envUpdate.newEnvironment);

				Packets::EntityEnvironmentUpdate envUpdatePacket;
				envUpdatePacket.tickIndex = tickIndex;
				envUpdatePacket.entity = envUpdate.entityId;
				envUpdatePacket.newEnvironmentId = envIndex;

				m_networkSession->SendPacket(envUpdatePacket);
			}
			m_environmentUpdates.clear();
		}

		Packets::EntitiesStateUpdate stateUpdate;
		stateUpdate.tickIndex = tickIndex;
		stateUpdate.lastInputIndex = m_lastInputIndex;

		if (m_controlledCharacter)
		{
			const Nz::EulerAnglesf& cameraRotation = m_controlledCharacter->GetCameraRotation();

			auto& controlledData = stateUpdate.controlledCharacter.emplace();
			controlledData.cameraPitch = cameraRotation.pitch;
			controlledData.cameraYaw = cameraRotation.yaw;
			controlledData.position = m_controlledCharacter->GetCharacterPosition();
			controlledData.referenceRotation = m_controlledCharacter->GetReferenceRotation();
		}

		for (const entt::handle& handle : m_movingEntities)
		{
			auto& entityData = stateUpdate.entities.emplace_back();

			auto& entityNode = handle.get<Nz::NodeComponent>();

			entityData.entityId = Nz::Retrieve(m_entityIndices, handle);

			entityData.newStates.position = entityNode.GetPosition();
			entityData.newStates.rotation = entityNode.GetRotation();
		}

		if (!stateUpdate.entities.empty() || stateUpdate.controlledCharacter.has_value())
			m_networkSession->SendPacket(stateUpdate);
	}

	void SessionVisibilityHandler::DispatchEnvironments(Nz::UInt16 tickIndex)
	{
		if (!m_destroyedEnvironments.empty())
		{
			for (ServerEnvironment* environment : m_destroyedEnvironments)
			{
				EnvironmentId envId = Nz::Retrieve(m_environmentIndices, environment);

				m_freeEnvironmentIds.Set(envId, true);

				auto& visibleEnvironment = m_visibleEnvironments[envId];
				for (std::size_t entityIndex : visibleEnvironment.entities.IterBits())
				{
					auto& entityData = m_visibleEntities[entityIndex];

					m_createdEntities.erase(entityData.entity);
					m_deletedEntities.erase(entityData.entity);
					m_movingEntities.erase(entityData.entity);
					m_entityIndices.erase(entityData.entity);
					m_freeEntityIds.Set(entityIndex, true);

					entityData.entity = entt::handle{};
					entityData.envIndex = std::numeric_limits<EnvironmentId>::max();
				}

				for (std::size_t chunkIndex : visibleEnvironment.chunks.IterBits())
				{
					auto& visibleChunk = m_visibleChunks[chunkIndex];
					visibleChunk.chunk = nullptr;
					visibleChunk.entityOwner = entt::handle{};
					visibleChunk.onBlockUpdatedSlot.Disconnect();

					m_freeChunkIds.Set(chunkIndex);
					m_resetChunk.UnboundedReset(chunkIndex);
					m_updatedChunk.UnboundedReset(chunkIndex);
				}

				m_environmentIndices.erase(environment);

				Packets::EnvironmentDestroy destroyPacket;
				destroyPacket.id = envId;
				destroyPacket.tickIndex = tickIndex;
				m_networkSession->SendPacket(destroyPacket);
			}

			m_destroyedEnvironments.clear();
		}

		if (!m_createdEnvironments.empty())
		{
			for (EnvironmentTransformation& environment : m_createdEnvironments)
			{
				std::size_t envIndex = m_freeEnvironmentIds.FindFirst();
				if (envIndex == m_freeEnvironmentIds.npos)
				{
					envIndex = m_freeEnvironmentIds.GetSize();
					m_freeEnvironmentIds.Resize(envIndex + FreeNetworkIdGrowRate, true);
				}

				m_freeEnvironmentIds.Set(envIndex, false);

				m_environmentIndices[environment.environment] = Nz::SafeCast<EnvironmentId>(envIndex);

				if (envIndex >= m_visibleEnvironments.size())
					m_visibleEnvironments.resize(envIndex + 1);

				Packets::EnvironmentCreate createPacket;
				createPacket.id = Nz::SafeCast<Nz::UInt8>(envIndex);
				createPacket.tickIndex = tickIndex;
				createPacket.transform = environment.transform;

				m_networkSession->SendPacket(createPacket);
			}

			m_createdEnvironments.clear();
		}

		if (!m_environmentTransformations.empty())
		{
			for (auto& envTransformation : m_environmentTransformations)
			{
				EnvironmentId envId = Nz::Retrieve(m_environmentIndices, envTransformation.environment);

				Packets::EnvironmentUpdate envMovementPacket;
				envMovementPacket.tickIndex = tickIndex;
				envMovementPacket.id = envId;
				envMovementPacket.transform = envTransformation.transform;

				m_networkSession->SendPacket(envMovementPacket);
			}
			m_environmentTransformations.clear();
		}

		if (m_nextRootEnvironment)
		{
			m_currentEnvironmentId = Nz::Retrieve(m_environmentIndices, m_nextRootEnvironment);

			Packets::UpdateRootEnvironment rootUpdatePacket;
			rootUpdatePacket.newRootEnv = m_currentEnvironmentId;

			m_networkSession->SendPacket(rootUpdatePacket);

			m_nextRootEnvironment = nullptr;
		}
	}
}
