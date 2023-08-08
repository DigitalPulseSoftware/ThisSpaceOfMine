// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/SessionVisibilityHandler.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <NazaraUtils/Algorithm.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>

namespace tsom
{
	void SessionVisibilityHandler::CreateChunk(Chunk& chunk)
	{
		// Check if this chunk was marked for destruction
		if (auto it = m_chunkIndices.find(&chunk); it != m_chunkIndices.end())
		{
			// Chunk still exists, resurrect it
			m_newlyHiddenChunk.Reset(it->second);
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
			m_chunkIndices.emplace(&chunk, chunkIndex);

			if (chunkIndex >= m_visibleChunks.size())
				m_visibleChunks.resize(chunkIndex + 1);

			VisibleChunk& chunkData = m_visibleChunks[chunkIndex];
			chunkData.chunk = &chunk;
			chunkData.chunkUpdatePacket.chunkId = Nz::SafeCast<Packets::Helper::ChunkId>(chunkIndex);
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

	void SessionVisibilityHandler::CreateEntity(entt::handle entity, CreateEntityData entityData)
	{
		if (entityData.isMoving)
			m_movingEntities.emplace(entity);

		m_createdEntities.emplace(entity, std::move(entityData));
	}

	void SessionVisibilityHandler::DestroyEntity(entt::handle entity)
	{
		m_createdEntities.erase(entity);
		m_movingEntities.erase(entity);
		m_deletedEntities.emplace(entity);
	}

	void SessionVisibilityHandler::Dispatch()
	{
		DispatchChunks();
		DispatchEntities();
	}

	Chunk* SessionVisibilityHandler::GetChunkByIndex(std::size_t chunkIndex) const
	{
		if (chunkIndex >= m_visibleChunks.size())
			return nullptr;

		return m_visibleChunks[chunkIndex].chunk;
	}

	void SessionVisibilityHandler::DispatchChunks()
	{
		for (std::size_t chunkIndex = m_newlyHiddenChunk.FindFirst(); chunkIndex != m_newlyHiddenChunk.npos; chunkIndex = m_newlyHiddenChunk.FindNext(chunkIndex))
		{
			// Mark chunk index as free on dispatch to prevent chunk index reuse while resurrecting it
			m_freeChunkIds.Set(chunkIndex);
			m_updatedChunk.UnboundedReset(chunkIndex);

			VisibleChunk& visibleChunk = m_visibleChunks[chunkIndex];
			visibleChunk.chunk = nullptr;
			visibleChunk.onCellUpdatedSlot.Disconnect();

			Packets::ChunkDestroy chunkDestroyPacket;
			chunkDestroyPacket.chunkId = Nz::SafeCast<Packets::Helper::ChunkId>(chunkIndex);

			m_networkSession->SendPacket(chunkDestroyPacket);
		}
		m_newlyHiddenChunk.Clear();

		for (std::size_t chunkIndex = m_newlyVisibleChunk.FindFirst(); chunkIndex != m_newlyVisibleChunk.npos; chunkIndex = m_newlyVisibleChunk.FindNext(chunkIndex))
		{
			VisibleChunk& visibleChunk = m_visibleChunks[chunkIndex];

			// Connect update signal on dispatch to prevent updates made during the same tick to be sent as update
			visibleChunk.onCellUpdatedSlot.Connect(visibleChunk.chunk->OnBlockUpdated, [this, chunkIndex]([[maybe_unused]] Chunk* chunk, const Nz::Vector3ui& indices, VoxelBlock newBlock)
			{
				m_updatedChunk.UnboundedSet(chunkIndex);

				VisibleChunk& visibleChunk = m_visibleChunks[chunkIndex];
				assert(visibleChunk.chunk == chunk);

				auto comp = [](Packets::ChunkUpdate::BlockUpdate& blockUpdate, const Nz::Vector3ui& indices)
				{
					return Nz::Vector3ui(blockUpdate.voxelLoc.x, blockUpdate.voxelLoc.y, blockUpdate.voxelLoc.z) < indices;
				};

				auto it = std::lower_bound(visibleChunk.chunkUpdatePacket.updates.begin(), visibleChunk.chunkUpdatePacket.updates.end(), indices, comp);
				if (it == visibleChunk.chunkUpdatePacket.updates.end() || Nz::Vector3ui(it->voxelLoc.x, it->voxelLoc.y, it->voxelLoc.z) != indices)
				{
					visibleChunk.chunkUpdatePacket.updates.push_back({
						Packets::Helper::VoxelLocation{ Nz::SafeCast<Nz::UInt8>(indices.x), Nz::SafeCast<Nz::UInt8>(indices.y), Nz::SafeCast<Nz::UInt8>(indices.z) },
						Nz::SafeCast<Nz::UInt8>(newBlock)
					});
				}
				else
					it->newContent = Nz::SafeCast<Nz::UInt8>(newBlock);
			});

			Nz::Vector3ui chunkLocation = visibleChunk.chunk->GetIndices();
			Nz::Vector3ui chunkSize = visibleChunk.chunk->GetSize();

			Packets::ChunkCreate chunkCreatePacket;
			chunkCreatePacket.chunkId = Nz::SafeCast<Packets::Helper::ChunkId>(chunkIndex);
			chunkCreatePacket.chunkLocX = chunkLocation.x;
			chunkCreatePacket.chunkLocY = chunkLocation.y;
			chunkCreatePacket.chunkLocZ = chunkLocation.z;
			chunkCreatePacket.chunkSizeX = chunkSize.x;
			chunkCreatePacket.chunkSizeY = chunkSize.y;
			chunkCreatePacket.chunkSizeZ = chunkSize.z;
			chunkCreatePacket.cellSize = visibleChunk.chunk->GetCellSize();

			unsigned int blockCount = chunkSize.x * chunkSize.y * chunkSize.z;
			chunkCreatePacket.content.resize(blockCount);

			const VoxelBlock* chunkContent = visibleChunk.chunk->GetContent();
			for (unsigned int i = 0; i < blockCount; ++i)
				chunkCreatePacket.content[i] = Nz::SafeCast<Nz::UInt8>(chunkContent[i]);

			m_networkSession->SendPacket(chunkCreatePacket);
		}
		m_newlyVisibleChunk.Clear();

		for (std::size_t chunkIndex = m_updatedChunk.FindFirst(); chunkIndex != m_updatedChunk.npos; chunkIndex = m_updatedChunk.FindNext(chunkIndex))
		{
			VisibleChunk& visibleChunk = m_visibleChunks[chunkIndex];
			m_networkSession->SendPacket(visibleChunk.chunkUpdatePacket);
		}
		m_updatedChunk.Clear();
	}

	void SessionVisibilityHandler::DispatchEntities()
	{
		if (!m_deletedEntities.empty())
		{
			Packets::EntitiesDelete deletePacket;

			for (const entt::handle& handle : m_deletedEntities)
			{
				Nz::UInt32 entityId = Nz::Retrieve(m_entityToNetworkId, handle);
				deletePacket.entities.push_back(entityId);

				m_freeEntityIds.Set(entityId, true);

				m_entityToNetworkId.erase(handle);
			}

			m_networkSession->SendPacket(deletePacket);
			m_deletedEntities.clear();
		}

		if (!m_createdEntities.empty())
		{
			Packets::EntitiesCreation creationPacket;

			for (auto&& [handle, data] : m_createdEntities)
			{
				std::size_t networkId = m_freeEntityIds.FindFirst();
				if (networkId == m_freeEntityIds.npos)
				{
					networkId = m_freeEntityIds.GetSize();
					m_freeEntityIds.Resize(networkId + FreeEntityIdGrowRate, true);
				}

				m_freeEntityIds.Set(networkId, false);

				m_entityToNetworkId[handle] = networkId;

				auto& entityData = creationPacket.entities.emplace_back();
				entityData.entityId = Nz::SafeCast<Nz::UInt32>(networkId);
				entityData.initialStates.position = data.initialPosition;
				entityData.initialStates.rotation = data.initialRotation;
				entityData.playerControlled = data.playerControlledData;
			}

			m_networkSession->SendPacket(creationPacket);
			m_createdEntities.clear();
		}

		Packets::EntitiesStateUpdate stateUpdate;

		for (const entt::handle& handle : m_movingEntities)
		{
			auto& entityData = stateUpdate.entities.emplace_back();

			auto& entityNode = handle.get<Nz::NodeComponent>();

			entityData.entityId = Nz::Retrieve(m_entityToNetworkId, handle);

			entityData.newStates.position = entityNode.GetPosition();
			entityData.newStates.rotation = entityNode.GetRotation();
		}

		if (!stateUpdate.entities.empty())
			m_networkSession->SendPacket(stateUpdate);
	}
}
