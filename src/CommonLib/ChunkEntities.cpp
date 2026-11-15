// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ComplexCollisionComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <CommonLib/Physics/ContactCallbackComponents.hpp>
#include <CommonLib/Systems/BuoyancySystem.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Core.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/HardwareInfo.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <cassert>

namespace tsom
{
	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, std::shared_ptr<ChunkContainer> chunkContainer, const BlockLibrary& blockLibrary, std::size_t layerIndex, bool isStatic) :
	ChunkEntities(application, world, std::move(chunkContainer), blockLibrary, layerIndex, isStatic, NoInit{})
	{
		FillChunks();
	}

	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, std::shared_ptr<ChunkContainer> chunkContainer, const BlockLibrary& blockLibrary, std::size_t layerIndex, bool isStatic, NoInit) :
	m_layerIndex(layerIndex),
	m_chunkContainer(std::move(chunkContainer)),
	m_application(application),
	m_world(world),
	m_blockLibrary(blockLibrary),
	m_isDynamicColliderUpdated(true),
	m_isStatic(isStatic),
	m_isSwitchingContainer(false)
	{
		auto& layerData = m_blockLibrary.GetLayerData(m_layerIndex);

		// Fluids require dynamic colliders to properly detect entities inside of them
		m_requiresDynamicColliders = !m_isStatic || layerData.isFluid;

		m_maxConcurrentJobs = std::max(Nz::Core::Instance()->GetHardwareInfo().GetCpuThreadCount() * 2, 4u);

		BindChunkContainerSignals();
	}

	ChunkEntities::~ChunkEntities()
	{
		Clear();
	}

	void ChunkEntities::BindChunkContainerSignals()
	{
		m_onChunkAdded.Connect(m_chunkContainer->OnChunkLayerAdded, [this](ChunkContainer* /*emitter*/, Chunk* chunk, std::size_t layerIndex)
		{
			if (m_layerIndex != layerIndex)
				return;

			// Chunks can be added from another thread
			std::lock_guard lock(m_chunkLock);
			m_createdDestroyedChunks[chunk->GetIndices()] = true;
		});

		m_onChunkRemove.Connect(m_chunkContainer->OnChunkLayerRemove, [this](ChunkContainer* /*emitter*/, Chunk* chunk, std::size_t layerIndex)
		{
			if (m_layerIndex != layerIndex)
				return;

			// Chunks can be added from another thread
			std::lock_guard lock(m_chunkLock);
			m_createdDestroyedChunks[chunk->GetIndices()] = false;
		});

		m_onChunkUpdated.Connect(m_chunkContainer->OnChunkUpdated, [this](ChunkContainer* /*emitter*/, Chunk* chunk, NeighborChunkMask neighborMask, Nz::UInt32 layerMask)
		{
			if ((layerMask & (1u << m_layerIndex)) == 0)
				return;

			// Chunks can be updated in parallel (e.g. planet generation)
			std::lock_guard lock(m_invalidatedChunkMutex);
			m_invalidatedChunks[chunk->GetIndices()] |= neighborMask;
		});
	}

	void ChunkEntities::ForEachChunk(Nz::FunctionRef<void(const ChunkIndices& chunkIndices, entt::handle chunkEntity)> callback)
	{
		for (auto it = m_chunkEntities.begin(); it != m_chunkEntities.end(); ++it)
			callback(it.key(), it.value());
	}

	void ChunkEntities::SetParentEntity(entt::handle entity)
	{
		m_parentEntity = entity;
		if (m_isStatic)
		{
			if (m_parentEntity)
			{
				auto& parentNode = m_parentEntity.get<Nz::NodeComponent>();
				m_onParentNodeInvalidated.Connect(parentNode.OnNodeInvalidation, this, &ChunkEntities::OnParentNodeInvalidated);
			}
			else
				m_onParentNodeInvalidated.Disconnect();
		}
	}

	void ChunkEntities::Update()
	{
		if (!m_chunkContainer)
			return;

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

			if (m_updateJobs.size() + m_invalidatedChunks.size() > m_maxConcurrentJobs)
			{
				for (auto it = m_invalidatedChunks.begin(); it != m_invalidatedChunks.end(); ++it)
				{
					UpdateChunkEntity(it.key(), it.value());
					if (m_updateJobs.size() >= m_maxConcurrentJobs)
					{
						m_invalidatedChunks.erase(m_invalidatedChunks.begin(), it);
						break;
					}
				}

				m_invalidatedChunks.clear(); //< just in case
			}
			else
			{
				for (auto&& [chunkIndices, neighborMask] : m_invalidatedChunks)
					UpdateChunkEntity(chunkIndices, neighborMask);

				m_invalidatedChunks.clear();
			}
		}

		if (m_isSwitchingContainer && m_updateJobs.empty())
		{
			ClearChunkEntities(m_previousChunkEntities);
			m_previousChunkEntities = decltype(m_previousChunkEntities){}; //< force the map to release its memory
			for (auto it = m_chunkEntities.begin(); it != m_chunkEntities.end(); ++it)
			{
				entt::handle entity = it.value();
				if (entity)
					entity.remove<Nz::DisabledComponent>();
			}

			m_isSwitchingContainer = false;
		}

		if (!m_isDynamicColliderUpdated)
		{
			NazaraAssert(!m_isStatic);

			std::vector<Nz::CompoundCollider3D::ChildCollider> childDynamicColliders;
			childDynamicColliders.reserve(m_chunkColliders.size());

			std::vector<ComplexChunkCollisionComponent::ChunkEntry> chunkEntries;
			chunkEntries.reserve(m_chunkColliders.size());

			for (auto&& [chunkIndices, chunkColliders] : m_chunkColliders)
			{
				if (!chunkColliders.dynamicCollider)
					continue; //< can happen with empty chunks

				auto& childDynamicCollider = childDynamicColliders.emplace_back();
				childDynamicCollider.collider = chunkColliders.dynamicCollider;
				childDynamicCollider.offset = m_chunkContainer->GetChunkPosition(chunkIndices);

				auto& chunkEntry = chunkEntries.emplace_back();
				chunkEntry.chunkCollider = chunkColliders.complexCollider;
				chunkEntry.chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);
				chunkEntry.chunkOffset = childDynamicCollider.offset;
			}

			auto& rigidBody = m_parentEntity.get<Nz::RigidBody3DComponent>();
			if (!childDynamicColliders.empty())
			{
				rigidBody.SetCollider(std::make_shared<Nz::CompoundCollider3D>(std::move(childDynamicColliders)));

				auto& complexChunkCollisions = m_parentEntity.get_or_emplace<ComplexChunkCollisionComponent>();
				complexChunkCollisions.chunkEntries = std::move(chunkEntries);
			}
			else
			{
				rigidBody.SetCollider(nullptr);
				m_parentEntity.remove<ComplexChunkCollisionComponent>();
			}

			m_isDynamicColliderUpdated = true;
		}
	}

	void ChunkEntities::UpdateChunkContainer(std::shared_ptr<ChunkContainer> chunkContainer)
	{
		m_chunkContainer = std::move(chunkContainer);

		// When switching containers, keep previous chunk entities alive until the switch is complete (e.g. until every chunk has been processed)
		if (!m_isSwitchingContainer)
		{
			m_isSwitchingContainer = true;
			m_previousChunkEntities = std::move(m_chunkEntities);
		}

		Clear();

		if (m_chunkContainer)
		{
			BindChunkContainerSignals();
			FillChunks();
		}
	}

	void ChunkEntities::Clear()
	{
		ClearChunkEntities(m_chunkEntities);
		m_invalidatedChunks.clear();
		m_finishedJobs.clear();

		for (auto it = m_updateJobs.begin(); it != m_updateJobs.end(); ++it)
			it->second->cancelled = true;

		m_updateJobs.clear();
		m_chunkColliders.clear();
		m_createdDestroyedChunks.clear();
	}

	void ChunkEntities::ClearChunkEntities(tsl::hopscotch_map<ChunkIndices, entt::handle>& chunkEntities)
	{
		for (auto it = chunkEntities.begin(); it != chunkEntities.end(); ++it)
		{
			entt::handle entity = it.value();
			if (entity)
				entity.destroy();
		}
		chunkEntities.clear();
	}

	void ChunkEntities::CreateChunkEntity(const ChunkIndices& chunkIndices)
	{
		// Entity may still exist if the chunk was destroyed and re-created in a row
		entt::handle chunkEntity;
		if (auto it = m_chunkEntities.find(chunkIndices); it != m_chunkEntities.end())
			chunkEntity = it->second;
		else
		{
			chunkEntity = m_world.CreateEntity();
			m_chunkEntities.emplace(chunkIndices, chunkEntity);
		}

		if (m_isSwitchingContainer)
			chunkEntity.emplace<Nz::DisabledComponent>();

		auto& nodeComponent = chunkEntity.emplace<Nz::NodeComponent>(m_chunkContainer->GetChunkPosition(chunkIndices));
		if (m_parentEntity)
		{
			m_parentEntity.get_or_emplace<EntityOwnerComponent>().Register(chunkEntity);
			nodeComponent.SetParent(m_parentEntity);
		}

		Chunk* chunk = m_chunkContainer->GetChunk(chunkIndices);
		NazaraAssert(chunk);

		auto& chunkComponent = chunkEntity.emplace<ChunkComponent>();
		chunkComponent.chunk = chunk;
		chunkComponent.parentEntity = m_parentEntity;

		auto& layerData = m_blockLibrary.GetLayerData(m_layerIndex);

		if (m_isStatic)
		{
			// For static collisions, keep a collider per chunk to facilitate updates
			Nz::RigidBody3D::StaticSettings physicsSettings(nullptr);
			physicsSettings.objectLayer = layerData.physicsLayer;
			physicsSettings.isTrigger = layerData.isPhysicsTrigger;

			chunkEntity.emplace<Nz::RigidBody3DComponent>(physicsSettings);
		}

		if (layerData.isFluid)
		{
			chunkEntity.emplace<Physics::ContactAddedCallbackComponent>().callback = &BuoyancySystem::HandleContactAdded;
			chunkEntity.emplace<Physics::ContactPersistedCallbackComponent>().callback = &BuoyancySystem::HandleContactPersisted;
			chunkEntity.emplace<Physics::ContactRemovedCallbackComponent>().callback = &BuoyancySystem::HandleContactRemoved;
		}

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
		if (!m_isStatic)
		{
			if (m_chunkColliders.erase(chunkIndices) > 0)
				m_isDynamicColliderUpdated = true;
		}
	}

	void ChunkEntities::FillChunks()
	{
		m_chunkContainer->ForEachChunk([this](const ChunkIndices& chunkIndices, Chunk& chunk)
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
		updateJob->chunk = chunk.shared_from_this();
		updateJob->chunkContainer = m_chunkContainer;

		updateJob->applyFunc = [this](const ChunkIndices& chunkIndices, UpdateJob&& job)
		{
			ColliderUpdateJob&& colliderUpdateJob = static_cast<ColliderUpdateJob&&>(job);

			UpdateChunkCollider(chunkIndices, std::move(colliderUpdateJob.collider));
		};

		updateJob->taskCount = 0;
		if (chunk.HasContent())
		{
			auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();
			taskScheduler.AddTask([this, updateJob]
			{
				if (updateJob->cancelled)
					return;

				ChunkReadLock lock(updateJob->chunk.get());
				if (updateJob->chunk->HasContent()) //< We need to re-check because the chunk may have lost its content (become full empty) since task was scheduled
					updateJob->collider = updateJob->chunk->BuildCollider(m_layerIndex, m_requiresDynamicColliders);

				updateJob->jobDone++;
			});
			updateJob->taskCount++;
		}

		// Add neighbor chunks
		for (NeighborChunk neighborChunk : neighborMask)
		{
			ChunkIndices neighborIndices = chunk.GetIndices() + s_neighborChunkOffset[neighborChunk];
			const Chunk* neighborChunkPtr = m_chunkContainer->GetChunk(neighborIndices);

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

	void ChunkEntities::UpdateChunkCollider(const ChunkIndices& chunkIndices, Chunk::ColliderData&& colliderData)
	{
		if (m_isStatic)
		{
			entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);
			auto& rigidBody = chunkEntity.get<Nz::RigidBody3DComponent>();
			if (colliderData.dynamicCollider)
			{
				rigidBody.SetCollider(std::move(colliderData.dynamicCollider), false);

				if (colliderData.complexCollider)
					chunkEntity.get_or_emplace<ComplexCollisionComponent>().complexCollisionCollider = std::move(colliderData.complexCollider);
				else
					chunkEntity.remove<ComplexCollisionComponent>();
			}
			else
				rigidBody.SetCollider(std::move(colliderData.complexCollider), false);
		}
		else
		{
			m_chunkColliders[chunkIndices] = std::move(colliderData);
			m_isDynamicColliderUpdated = false;
		}
	}
}
