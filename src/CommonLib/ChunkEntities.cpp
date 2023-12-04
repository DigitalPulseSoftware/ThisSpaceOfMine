// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltRigidBody3DComponent.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <cassert>

namespace tsom
{
	ChunkEntities::ChunkEntities(Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary) :
	ChunkEntities(world, chunkContainer, blockLibrary, NoInit{})
	{
		FillChunks();
	}
	
	ChunkEntities::ChunkEntities(Nz::EnttWorld& world, const ChunkContainer& chunkContainer, const BlockLibrary& blockLibrary, NoInit) :
	m_world(world),
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
		for (std::size_t chunkId = m_invalidatedChunks.FindFirst(); chunkId != m_invalidatedChunks.npos; chunkId = m_invalidatedChunks.FindNext(chunkId))
			UpdateChunkEntity(chunkId);

		m_invalidatedChunks.Clear();
	}

	void ChunkEntities::CreateChunkEntity(std::size_t chunkId, const Chunk* chunk)
	{
		m_chunkEntities[chunkId] = m_world.CreateEntity();
		m_chunkEntities[chunkId].emplace<Nz::NodeComponent>(m_chunkContainer.GetChunkOffset(chunk->GetIndices()));
		m_chunkEntities[chunkId].emplace<Nz::JoltRigidBody3DComponent>(Nz::JoltRigidBody3D::StaticSettings(chunk->BuildCollider(m_blockLibrary)));
	}

	void ChunkEntities::DestroyChunkEntity(std::size_t chunkId)
	{
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

	void ChunkEntities::UpdateChunkEntity(std::size_t chunkId)
	{
		assert(m_chunkEntities[chunkId]);
		const Chunk* chunk = m_chunkContainer.GetChunk(chunkId);
		assert(chunk);

		std::shared_ptr<Nz::JoltCollider3D> newCollider = chunk->BuildCollider(m_blockLibrary);

		auto& rigidBody = m_chunkEntities[chunkId].get<Nz::JoltRigidBody3DComponent>();
		rigidBody.SetGeom(std::move(newCollider), false);
	}
}
