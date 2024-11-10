// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <cassert>

namespace tsom
{
	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary) :
	ChunkEntities(application, world, chunkContainer, blockLibrary, NoInit{})
	{
		FillChunks();
	}

	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, NoInit) :
	m_application(application),
	m_world(world),
	m_blockLibrary(blockLibrary),
	m_chunkContainer(chunkContainer)
	{
		m_onChunkAdded.Connect(chunkContainer.OnChunkAdded, [this](ChunkContainer* /*emitter*/, Chunk* chunk)
		{
			CreateChunkEntity(chunk->GetIndices(), *chunk);
		});

		m_onChunkRemove.Connect(chunkContainer.OnChunkRemove, [this](ChunkContainer* /*emitter*/, Chunk* chunk)
		{
			DestroyChunkEntity(chunk->GetIndices());
		});

		m_onChunkUpdated.Connect(chunkContainer.OnChunkUpdated, [this](ChunkContainer* /*emitter*/, Chunk* chunk, DirectionMask neighborMask)
		{
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
				job.applyFunc(it->first, std::move(job));

				// Don't remove jobs immediatly to be able to detect dependencies errors
				m_finishedJobs.push_back(it->first);
			}
		}

		for (const ChunkIndices& indices : m_finishedJobs)
			m_updateJobs.erase(indices);
		m_finishedJobs.clear();

		for (auto&& [chunkIndices, neighborMask] : m_invalidatedChunks)
			UpdateChunkEntity(chunkIndices, neighborMask);

		m_invalidatedChunks.clear();
	}

	void ChunkEntities::CreateChunkEntity(const ChunkIndices& chunkIndices, Chunk& chunk)
	{
		entt::handle chunkEntity = m_world.CreateEntity();

		auto& nodeComponent = chunkEntity.emplace<Nz::NodeComponent>(m_chunkContainer.GetChunkOffset(chunkIndices));
		if (m_parentEntity)
		{
			m_parentEntity.get_or_emplace<EntityOwnerComponent>().Register(chunkEntity);
			nodeComponent.SetParent(m_parentEntity);
		}

		auto& chunkComponent = chunkEntity.emplace<ChunkComponent>();
		chunkComponent.chunk = &chunk;
		chunkComponent.parentEntity = m_parentEntity;

		Nz::RigidBody3D::StaticSettings physicsSettings(nullptr);
		physicsSettings.objectLayer = Constants::ObjectLayerStatic;

		chunkEntity.emplace<Nz::RigidBody3DComponent>(physicsSettings);

		assert(!m_chunkEntities.contains(chunkIndices));
		m_chunkEntities.insert_or_assign(chunkIndices, chunkEntity);

		if (chunk.HasContent())
			ProcessChunkUpdate(chunk, 0);
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
			CreateChunkEntity(chunkIndices, chunk);
		});
	}

	auto ChunkEntities::ProcessChunkUpdate(const Chunk& chunk, DirectionMask neighborMask) -> UpdateJob*
	{
		assert(chunk.HasContent());

		// Try to cancel current update job to avoid useless work
		if (auto it = m_updateJobs.find(chunk.GetIndices()); it != m_updateJobs.end())
		{
			UpdateJob& job = *it->second;
			job.cancelled = true;
		}

		std::shared_ptr<ColliderUpdateJob> updateJob = std::make_shared<ColliderUpdateJob>();
		updateJob->taskCount = 1;

		updateJob->applyFunc = [this](const ChunkIndices& chunkIndices, UpdateJob&& job)
		{
			ColliderUpdateJob&& colliderUpdateJob = static_cast<ColliderUpdateJob&&>(job);

			entt::handle chunkEntity = Nz::Retrieve(m_chunkEntities, chunkIndices);
			auto& rigidBody = chunkEntity.get<Nz::RigidBody3DComponent>();
			rigidBody.SetCollider(std::move(colliderUpdateJob.collider), false);
		};

		auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();
		taskScheduler.AddTask([this, updateJob, chunkPtr = chunk.shared_from_this()]
		{
			if (updateJob->cancelled)
				return;

			chunkPtr->LockRead();
			updateJob->collider = chunkPtr->BuildCollider();
			chunkPtr->UnlockRead();

			updateJob->jobDone++;
		});

		// Add neighbor chunks
		for (Direction neighborDir : neighborMask)
		{
			ChunkIndices neighborIndices = chunk.GetIndices() + s_chunkDirOffset[neighborDir];
			const Chunk* neighborChunk = m_chunkContainer.GetChunk(neighborIndices);

			// We only need to regenerate collisions for neighbor chunks having per-face collisions (like deformed chunks)
			if (!neighborChunk || !neighborChunk->HasContent() || !neighborChunk->HasPerFaceCollisions())
				continue;

			updateJob->chunkDependencies.push_back(neighborIndices);

			// Trigger our neighbor update
			if (!m_updateJobs.contains(neighborIndices))
				ProcessChunkUpdate(*neighborChunk, 0);
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
}
