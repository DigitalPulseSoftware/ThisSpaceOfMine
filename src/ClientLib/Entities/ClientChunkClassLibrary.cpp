// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>

namespace tsom
{
	ClientChunkClassLibrary::ClientChunkClassLibrary(Nz::ApplicationBase& app, const ClientBlockLibrary& blockLibrary) :
	ChunkClassLibrary(app, blockLibrary)
	{
	}

	void ClientChunkClassLibrary::InitializeChunkEntity(entt::handle entity)
	{
		entity.emplace<ChunkNetworkMapComponent>();
	}

	std::unique_ptr<ChunkEntities> ClientChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer)
	{
		return std::make_unique<ClientChunkEntities>(m_app, world, chunkContainer, Nz::SafeCast<const ClientBlockLibrary&>(m_blockLibrary));
	}
}
