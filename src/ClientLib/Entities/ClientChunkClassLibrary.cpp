// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Rendering/PlanetRenderable.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>

namespace tsom
{
	ClientChunkClassLibrary::ClientChunkClassLibrary(Nz::ApplicationBase& app, ConfigFile& config, const ClientBlockLibrary& blockLibrary) :
	ChunkClassLibrary(app, blockLibrary),
	m_config(config)
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

	std::shared_ptr<ChunkEntities> ClientChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, entt::handle parentEntity, ChunkContainer& chunkContainer, std::size_t layerIndex)
	{
		const ClientBlockLibrary& blockLibrary = Nz::SafeCast<const ClientBlockLibrary&>(m_blockLibrary);

		auto chunkEntities = std::make_shared<ClientChunkEntities>(m_app, m_config, world, parentEntity, chunkContainer, blockLibrary, layerIndex);

		auto& gfxComponent = parentEntity.get_or_emplace<Nz::GraphicsComponent>();
		if (!gfxComponent.GetRenderables().front().renderable)
		{
			gfxComponent.AttachRenderable(std::make_shared<PlanetRenderable>(blockLibrary, chunkContainer), Constants::RenderMask3D);
		}

		PlanetRenderable& planetRenderable = Nz::SafeCast<PlanetRenderable&>(*gfxComponent.GetRenderableEntry(0).renderable);
		planetRenderable.RegisterLayer(chunkEntities);

		return chunkEntities;
	}
}
