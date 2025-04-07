// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>

namespace tsom
{
	ClientChunkClassLibrary::ClientChunkClassLibrary(Nz::ApplicationBase& app, const ClientBlockLibrary& blockLibrary) :
	ChunkClassLibrary(app, blockLibrary)
	{
	}

	void ClientChunkClassLibrary::InitializePlanetEntity(entt::handle entity)
	{
		entity.emplace<ChunkNetworkMapComponent>();
		auto& atmosphereScattering = entity.emplace<AtmosphereScattering>();

		auto& entityInstance = entity.get<ClassInstanceComponent>();

		atmosphereScattering.planetCornerRadius = entityInstance.GetProperty<EntityPropertyType::Float>("CornerRadius");
		atmosphereScattering.planetDimensions = entityInstance.GetProperty<EntityPropertyType::FloatSize3D>("AtmospherePlanetDims");
	}

	void ClientChunkClassLibrary::InitializeShipEntity(entt::handle entity)
	{
		entity.emplace<ChunkNetworkMapComponent>();
	}

	std::unique_ptr<ChunkEntities> ClientChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer, std::size_t layerIndex)
	{
		return std::make_unique<ClientChunkEntities>(m_app, world, chunkContainer, Nz::SafeCast<const ClientBlockLibrary&>(m_blockLibrary), layerIndex);
	}
}
