// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <cassert>

namespace tsom
{
	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary) :
	ChunkEntities(application, world, chunkContainer, blockLibrary, NoInit{})
	{
		FillChunks();
	}

	ChunkEntities::ChunkEntities(Nz::ApplicationBase& application, Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, NoInit) :
	m_world(world),
	m_application(application),
	m_blockLibrary(blockLibrary),
	m_chunkContainer(chunkContainer)
	{
		m_chunkEntities.resize(chunkContainer.GetChunkCount());

		m_onChunkAdded.Connect(chunkContainer.OnChunkAdded, [this](ChunkContainer* /*emitter*/, Chunk* chunk)
		{
			std::size_t chunkId = m_chunkContainer.GetChunkIndex(chunk->GetIndices());
			CreateChunkEntity(chunkId, chunk);
		});

		m_onChunkRemove.Connect(chunkContainer.OnChunkRemove, [this](ChunkContainer* /*emitter*/, Chunk* chunk)
		{
			std::size_t chunkId = m_chunkContainer.GetChunkIndex(chunk->GetIndices());
			DestroyChunkEntity(chunkId);
		});

		m_onChunkUpdated.Connect(chunkContainer.OnChunkUpdated, [this](ChunkContainer* /*emitter*/, Chunk* chunk)
		{
			std::size_t chunkId = m_chunkContainer.GetChunkIndex(chunk->GetIndices());
			m_invalidatedChunks.UnboundedSet(chunkId);
		});
	}

	ChunkEntities::~ChunkEntities()
	{
		for (entt::handle entity : m_chunkEntities)
		{
			if (entity)
				entity.destroy();
		}
	}

	void ChunkEntities::Update()
	{
		for (auto it = m_updateJobs.begin(); it != m_updateJobs.end(); )
		{
			UpdateJob& job = *it->second;
			if (job.executionCounter != job.taskCount)
			{
				++it;
				continue;
			}

			job.applyFunc(it->first, std::move(job));
			it = m_updateJobs.erase(it);
		}

		for (std::size_t chunkId = m_invalidatedChunks.FindFirst(); chunkId != m_invalidatedChunks.npos; chunkId = m_invalidatedChunks.FindNext(chunkId))
			UpdateChunkEntity(chunkId);

		m_invalidatedChunks.Clear();
	}

	void ChunkEntities::CreateChunkEntity(std::size_t chunkId, const Chunk* chunk)
	{
		m_chunkEntities[chunkId] = m_world.CreateEntity();
		m_chunkEntities[chunkId].emplace<Nz::NodeComponent>(m_chunkContainer.GetChunkOffset(chunk->GetIndices()));
		m_chunkEntities[chunkId].emplace<Nz::RigidBody3DComponent>(Nz::RigidBody3D::StaticSettings(std::make_shared<Nz::SphereCollider3D>(1.f))); //< FIXME: null collider couldn't be changed back (sensor issue), set to null when Nazara is updated

		HandleChunkUpdate(chunkId, chunk);
	}

	void ChunkEntities::DestroyChunkEntity(std::size_t chunkId)
	{
		if (auto it = m_updateJobs.find(chunkId); it != m_updateJobs.end())
		{
			UpdateJob& job = *it->second;
			job.cancelled = true;

			m_updateJobs.erase(chunkId);
		}

		m_chunkEntities[chunkId].destroy();
		m_invalidatedChunks.UnboundedReset(chunkId);
	}

	void ChunkEntities::FillChunks()
	{
		for (std::size_t i = 0; i < m_chunkEntities.size(); ++i)
		{
			if (const Chunk* chunk = m_chunkContainer.GetChunk(i))
				CreateChunkEntity(i, chunk);
		}
	}

	void ChunkEntities::HandleChunkUpdate(std::size_t chunkId, const Chunk* chunk)
	{
		// Try to cancel current update job to void useless work
		if (auto it = m_updateJobs.find(chunkId); it != m_updateJobs.end())
		{
			UpdateJob& job = *it->second;
			job.cancelled = true;
		}

		std::shared_ptr<ColliderUpdateJob> updateJob = std::make_shared<ColliderUpdateJob>();
		updateJob->taskCount = 1;

		updateJob->applyFunc = [this](std::size_t chunkId, UpdateJob&& job)
		{
			ColliderUpdateJob&& colliderUpdateJob = static_cast<ColliderUpdateJob&&>(job);

			auto& rigidBody = m_chunkEntities[chunkId].get<Nz::RigidBody3DComponent>();
			rigidBody.SetGeom(std::move(colliderUpdateJob.collider), false);
		};

		auto& taskScheduler = m_application.GetComponent<Nz::TaskSchedulerAppComponent>();
		taskScheduler.AddTask([this, chunk, updateJob]
		{
			if (updateJob->cancelled)
				return;

			chunk->LockRead();
			updateJob->collider = chunk->BuildCollider(m_blockLibrary);
			chunk->UnlockRead();

			updateJob->executionCounter++;
		});

		m_updateJobs[chunkId] = std::move(updateJob);
	}

	void ChunkEntities::UpdateChunkEntity(std::size_t chunkId)
	{
		assert(m_chunkEntities[chunkId]);
		const Chunk* chunk = m_chunkContainer.GetChunk(chunkId);
		assert(chunk);

		HandleChunkUpdate(chunkId, chunk);
	}
}
