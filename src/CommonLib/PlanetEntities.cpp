// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/PlanetEntities.hpp>
#include <CommonLib/Planet.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltRigidBody3DComponent.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <cassert>

namespace tsom
{
	PlanetEntities::PlanetEntities(Nz::EnttWorld& world, const Planet& planet) :
	PlanetEntities(world, planet, NoInit{})
	{
		FillChunks();
	}
	
	PlanetEntities::PlanetEntities(Nz::EnttWorld& world, const Planet& planet, NoInit) :
	m_world(world),
	m_planet(planet)
	{
		m_chunkEntities.resize(planet.GetChunkCount());

		m_onChunkAdded.Connect(planet.OnChunkAdded, [this](Planet* /*emitter*/, Chunk* chunk)
		{
			std::size_t chunkId = m_planet.GetChunkIndex(chunk->GetIndices());
			CreateChunkEntity(chunkId, chunk);
		});

		m_onChunkRemove.Connect(planet.OnChunkRemove, [this](Planet* /*emitter*/, Chunk* chunk)
		{
			std::size_t chunkId = m_planet.GetChunkIndex(chunk->GetIndices());
			DestroyChunkEntity(chunkId);
		});

		m_onChunkUpdated.Connect(planet.OnChunkUpdated, [this](Planet* /*emitter*/, Chunk* chunk)
		{
			std::size_t chunkId = m_planet.GetChunkIndex(chunk->GetIndices());
			m_invalidatedChunks.UnboundedSet(chunkId);
		});
	}

	PlanetEntities::~PlanetEntities()
	{
		for (entt::handle entity : m_chunkEntities)
		{
			if (entity)
				entity.destroy();
		}
	}

	void PlanetEntities::Update()
	{
		for (std::size_t chunkId = m_invalidatedChunks.FindFirst(); chunkId != m_invalidatedChunks.npos; chunkId = m_invalidatedChunks.FindNext(chunkId))
			UpdateChunkEntity(chunkId);

		m_invalidatedChunks.Clear();
	}

	void PlanetEntities::CreateChunkEntity(std::size_t chunkId, const Chunk* chunk)
	{
		m_chunkEntities[chunkId] = m_world.CreateEntity();
		m_chunkEntities[chunkId].emplace<Nz::NodeComponent>(m_planet.GetChunkOffset(chunk->GetIndices()));
		m_chunkEntities[chunkId].emplace<Nz::JoltRigidBody3DComponent>(Nz::JoltRigidBody3D::StaticSettings(chunk->BuildCollider()));
	}

	void PlanetEntities::DestroyChunkEntity(std::size_t chunkId)
	{
		m_chunkEntities[chunkId].destroy();
		m_invalidatedChunks.UnboundedReset(chunkId);
	}

	void PlanetEntities::FillChunks()
	{
		for (std::size_t i = 0; i < m_chunkEntities.size(); ++i)
		{
			if (const Chunk* chunk = m_planet.GetChunk(i))
				CreateChunkEntity(i, chunk);
		}
	}

	void PlanetEntities::UpdateChunkEntity(std::size_t chunkId)
	{
		assert(m_chunkEntities[chunkId]);
		const Chunk* chunk = m_planet.GetChunk(chunkId);
		assert(chunk);

		std::shared_ptr<Nz::JoltCollider3D> newCollider = chunk->BuildCollider();

		auto& rigidBody = m_chunkEntities[chunkId].get<Nz::JoltRigidBody3DComponent>();
		rigidBody.SetGeom(std::move(newCollider), false);
	}
}
