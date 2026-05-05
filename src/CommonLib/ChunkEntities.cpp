// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <CommonLib/Physics/ContactCallbackComponents.hpp>
#include <CommonLib/Systems/BuoyancySystem.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <cassert>

namespace tsom
{
	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, std::size_t layerIndex) :
	ChunkEntities(application, world, chunkContainer, blockLibrary, layerIndex, NoInit{})
	{
		FillChunks();
	}

	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, std::size_t layerIndex, NoInit) :
	m_layerIndex(layerIndex),
	m_application(application),
	m_world(world),
	m_blockLibrary(blockLibrary),
	m_chunkContainer(chunkContainer)
	{
		m_onChunkAdded.Connect(chunkContainer.OnChunkLayerAdded, [this](ChunkContainer* /*emitter*/, Chunk* chunk, std::size_t layerIndex)
		{
			if (m_layerIndex != layerIndex)
				return;

			// Chunks can be added from another thread
			std::lock_guard lock(m_chunkLock);
			m_createdDestroyedChunks[chunk->GetIndices()] = true;
		});

		m_onChunkRemove.Connect(chunkContainer.OnChunkLayerRemove, [this](ChunkContainer* /*emitter*/, Chunk* chunk, std::size_t layerIndex)
		{
			if (m_layerIndex != layerIndex)
				return;

			// Chunks can be added from another thread
			std::lock_guard lock(m_chunkLock);
			m_createdDestroyedChunks[chunk->GetIndices()] = false;
		});

		m_onChunkUpdated.Connect(chunkContainer.OnChunkUpdated, [this](ChunkContainer* /*emitter*/, Chunk* chunk, NeighborChunkMask neighborMask, Nz::UInt32 layerMask)
		{
			if ((layerMask & (1u << m_layerIndex)) == 0)
				return;

			// Chunks can be updated in parallel (e.g. planet generation)
			std::lock_guard lock(m_invalidatedChunkMutex);
			m_invalidatedChunks[chunk->GetIndices()] |= neighborMask;
		});
	}

	ChunkEntities::~ChunkEntities()
	{
		for (auto it = m_chunkEntities.begin(); it != m_chunkEntities.end(); ++it)
		{
			entt::handle entity = it.value();
			if (entity)
				entity.destroy();
		}
	}

	void ChunkEntities::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, entt::handle chunkEntity)> callback)
	{
		for (auto it = m_chunkEntities.begin(); it != m_chunkEntities.end(); ++it)
			callback(it.key(), it.value());
	}

	void ChunkEntities::SetParentEntity(entt::handle entity)
	{
		m_parentEntity = entity;
		if (m_parentEntity)
		{
			auto& parentNode = m_parentEntity.get<Nz::NodeComponent>();
			m_onParentNodeInvalidated.Connect(parentNode.OnNodeInvalidation, this, &ChunkEntities::OnParentNodeInvalidated);
		}
		else
			m_onParentNodeInvalidated.Disconnect();
	}

	void ChunkEntities::Update()
	{
		for (auto it = m_updateJobs.begin(); it != m_updateJobs.end(); ++it)
		{
			UpdateJob& job = *it->second;
			if (!job.HasFinished())
				continue;

			bool canExecute = true;
			for (auto depIt = job.chunkDependencies.begin(); depIt != job.chunkDependencies.end();)
			{
				auto depJobIt = m_updateJobs.find(*depIt);
				if (depJobIt == m_updateJobs.end() || depJobIt->second->HasFinished())
				{
					depIt = job.chunkDependencies.erase(depIt);
					continue;
				}

				canExecute = false;
				++depIt;
			}

			if (canExecute)
			{
				// Don't remove jobs immediately to be able to detect dependencies errors
				m_finishedJobs.emplace_back(FinishedJob{ it->first, it->second });
			}
		}

		for (auto&& [indices, job] : m_finishedJobs)
		{
			job->applyFunc(indices, std::move(*job));
			m_updateJobs.erase(indices);
		}
		m_finishedJobs.clear();

		{
			std::lock_guard lock(m_chunkLock);

			for (auto&& [chunkIndices, created] : m_createdDestroyedChunks)
			{
				if (created)
					CreateChunkEntity(chunkIndices);
				else
					DestroyChunkEntity(chunkIndices);
			}
			m_createdDestroyedChunks.clear();

			std::lock_guard lock2(m_invalidatedChunkMutex);

			for (auto&& [chunkIndices, neighborMask] : m_invalidatedChunks)
				UpdateChunkEntity(chunkIndices, neighborMask);

			m_invalidatedChunks.clear();
		}
	}

	void ChunkEntities::CreateChunkEntity(const ChunkIndices& chunkIndices)
	{
		entt::handle chunkEntity = m_world.CreateEntity();

		auto& nodeComponent = chunkEntity.emplace<Nz::NodeComponent>(m_chunkContainer.GetChunkOffset(chunkIndices));
		if (m_parentEntity)
		{
			m_parentEntity.get_or_emplace<EntityOwnerComponent>().Register(chunkEntity);
			nodeComponent.SetParent(m_parentEntity);
		}

		Chunk* chunk = m_chunkContainer.GetChunk(chunkIndices);
		NazaraAssert(chunk);

		auto& chunkComponent = chunkEntity.emplace<ChunkComponent>();
		chunkComponent.chunk = chunk;
		chunkComponent.parentEntity = m_parentEntity;

		auto& layerData = m_blockLibrary.GetLayerData(m_layerIndex);

		Nz::RigidBody3D::StaticSettings physicsSettings(nullptr);
		physicsSettings.objectLayer = layerData.physicsLayer;
		physicsSettings.isTrigger = layerData.isPhysicsTrigger;

		chunkEntity.emplace<Nz::RigidBody3DComponent>(physicsSettings);

		if (layerData.isFluid)
		{
			chunkEntity.emplace<Physics::ContactAddedCallbackComponent>().callback = &BuoyancySystem::HandleContactAdded;
			chunkEntity.emplace<Physics::ContactPersistedCallbackComponent>().callback = &BuoyancySystem::HandleContactPersisted;
			chunkEntity.emplace<Physics::ContactRemovedCallbackComponent>().callback = &BuoyancySystem::HandleContactRemoved;
		}

		assert(!m_chunkEntities.contains(chunkIndices));
		m_chunkEntities.insert_or_assign(chunkIndices, chunkEntity);

		if (chunk->HasContent())
			ProcessChunkUpdate(*chunk, 0);
	}

	void ChunkEntities::DestroyChunkEntity(const ChunkIndices& chunkIndices)
	{
		if (auto it = m_updateJobs.find(chunkIndices); it != m_updateJobs.end())
		{
			UpdateJob& job = *it->second;
			job.cancelled = true;

			m_updateJobs.erase(chunkIndices);
		}

		if (auto it = m_chunkEntities.find(chunkIndices); it != m_chunkEntities.end())
		{
			it.value().destroy();
			m_chunkEntities.erase(it);
		}

		m_invalidatedChunks.erase(chunkIndices);
	}

	void ChunkEntities::FillChunks()
	{
		m_chunkContainer.ForEachChunk([this](const ChunkIndices& chunkIndices, Chunk& chunk)
		{
			if (chunk.IsLayerRegistered(m_layerIndex))
				CreateChunkEntity(chunkIndices);
		});
	}

	auto ChunkEntities::ProcessChunkUpdate(const Chunk& chunk, NeighborChunkMask neighborMask) -> UpdateJob*
	{
		NazaraAssert(m_chunkEntities.contains(chunk.GetIndices()));

		// Try to cancel current update job to avoid useless work
		if (auto it = m_updateJobs.find(chunk.GetIndices()); it != m_updateJobs.end())
		{
			UpdateJob& job = *it->second;
			job.cancelled = true;
		}

		std::shared_ptr<ColliderUpdateJob> updateJob = std::make_shared<ColliderUpdateJob>();

		updateJob->applyFunc = [this](const ChunkIndices& chunkIndices, UpdateJob&& job)
		{
			ColliderUpdateJob&& colliderUpdateJob = static_cast<ColliderUpdateJob&&>(job);

			entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);
			auto& rigidBody = chunkEntity.get<Nz::RigidBody3DComponent>();
			rigidBody.SetCollider(std::move(colliderUpdateJob.collider), false);
		};

		updateJob->taskCount = 0;
		if (chunk.HasContent())
		{
			auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();
			taskScheduler.AddTask([this, updateJob, chunkPtr = chunk.shared_from_this()]
			{
				if (updateJob->cancelled)
					return;

				ChunkReadLock lock(chunkPtr.get());
				if (chunkPtr->HasContent()) //< We need to re-check because the chunk may have lost its content (become full empty) since task was scheduled
					updateJob->collider = chunkPtr->BuildCollider(m_layerIndex);

				updateJob->jobDone++;
			});
			updateJob->taskCount++;
		}

		// Add neighbor chunks
		for (NeighborChunk neighborChunk : neighborMask)
		{
			ChunkIndices neighborIndices = chunk.GetIndices() + s_neighborChunkOffset[neighborChunk];
			const Chunk* neighborChunkPtr = m_chunkContainer.GetChunk(neighborIndices);

			// We only need to regenerate collisions for neighbor chunks having per-face collisions (like deformed chunks)
			if (!neighborChunkPtr || !neighborChunkPtr->HasContent() || !neighborChunkPtr->HasPerFaceCollisions() || !neighborChunkPtr->IsLayerRegistered(m_layerIndex))
				continue;

			updateJob->chunkDependencies.push_back(neighborIndices);

			// Trigger our neighbor update
			if (!m_updateJobs.contains(neighborIndices) && m_chunkEntities.contains(neighborIndices))
				ProcessChunkUpdate(*neighborChunkPtr, 0);
		}

		UpdateJob* updateJobPtr = updateJob.get();
		m_updateJobs.insert_or_assign(chunk.GetIndices(), std::move(updateJob));

		return updateJobPtr;
	}

	void ChunkEntities::OnParentNodeInvalidated(const Nz::Node* /*node*/)
	{
		// Refresh physical position
		for (auto it = m_chunkEntities.begin(); it != m_chunkEntities.end(); ++it)
		{
			entt::handle chunkEntity = it->second;

			// FIXME: This signal may be triggered while entities are destroyed
			if (!chunkEntity.all_of<Nz::NodeComponent, Nz::RigidBody3DComponent>())
				continue;

			auto& chunkNode = chunkEntity.get<Nz::NodeComponent>();
			auto& rigidBody = chunkEntity.get<Nz::RigidBody3DComponent>();
			rigidBody.TeleportTo(chunkNode.GetGlobalPosition(), chunkNode.GetGlobalRotation());
		}
	}

	void ChunkEntities::RebuildAllChunks()
	{
		for (auto it = m_updateJobs.begin(); it != m_updateJobs.end(); ++it)
		{
			it->second->cancelled = true;
		}
		m_updateJobs.clear();

		std::lock_guard lock(m_invalidatedChunkMutex);
		for (const auto& [chunkIndices, entity] : m_chunkEntities)
		{
			NazaraUnused(entity);
			m_invalidatedChunks[chunkIndices] = 0;
		}
	}
}
