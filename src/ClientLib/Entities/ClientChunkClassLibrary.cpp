// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ClientChunkClassLibrary::ClientChunkClassLibrary(Nz::ApplicationBase& app, ConfigFile& config, const ClientBlockLibrary& blockLibrary) :
	ChunkClassLibrary(app, blockLibrary),
	m_config(config)
	{
	}

	PlanetComponent& ClientChunkClassLibrary::InitializePlanetEntity(entt::handle entity, std::shared_ptr<Planet>&& planet)
	{
		PlanetComponent& planetComponent = ChunkClassLibrary::InitializePlanetEntity(entity, std::move(planet));

		entity.emplace<ChunkNetworkMapComponent>();

		auto& entityInstance = entity.get<ClassInstanceComponent>();

		const std::string& atmosphereShape = entityInstance.GetProperty<EntityPropertyType::String>("Atmosphere.Shape");
		if (!atmosphereShape.empty())
		{
			auto& atmosphereScattering = entity.emplace<AtmosphereScattering>();
			atmosphereScattering.shapeSettings = entityInstance.GetProperty<EntityPropertyType::Float4>("Atmosphere.ShapeSettings");
			atmosphereScattering.atmosphereMaxHeight = entityInstance.GetProperty<EntityPropertyType::Float>("Atmosphere.MaxHeight");
			atmosphereScattering.scatteringStrength = entityInstance.GetProperty<EntityPropertyType::Float>("Atmosphere.ScatteringStrength");
			atmosphereScattering.waveLengths = entityInstance.GetProperty<EntityPropertyType::Float3>("Atmosphere.WaveLengths");
			atmosphereScattering.mieHeight = entityInstance.GetProperty<EntityPropertyType::Float>("Atmosphere.MieHeight");
			atmosphereScattering.mieScattering = entityInstance.GetProperty<EntityPropertyType::Float>("Atmosphere.MieScattering");
			atmosphereScattering.densityFalloff = entityInstance.GetProperty<EntityPropertyType::Float>("Atmosphere.DensityFalloff");

			if (atmosphereShape == "RoundCube")
				atmosphereScattering.shape = AtmosphereScatteringShape::RoundCube;
			else if (atmosphereShape == "Torus")
				atmosphereScattering.shape = AtmosphereScatteringShape::Torus;
			else
			{
				spdlog::error("invalid atmosphere shape {}", atmosphereShape);
				entity.erase<AtmosphereScattering>();
			}
		}

		return planetComponent;
	}

	ShipComponent& ClientChunkClassLibrary::InitializeShipEntity(entt::handle entity, std::unique_ptr<Ship>&& ship)
	{
		ShipComponent& shipComponent = ChunkClassLibrary::InitializeShipEntity(entity, std::move(ship));

		entity.emplace<ChunkNetworkMapComponent>();

		return shipComponent;
	}

	std::unique_ptr<ChunkEntities> ClientChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer, std::size_t layerIndex)
	{
		return std::make_unique<ClientChunkEntities>(m_app, m_config, world, chunkContainer, Nz::SafeCast<const ClientBlockLibrary&>(m_blockLibrary), layerIndex);
	}
}
