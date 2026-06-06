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

		auto& atmosphereScattering = entity.emplace<AtmosphereScattering>();

		if (entityInstance.GetClass()->GetName() == "round_cube_planet")
		{
			Nz::Vector4f planetSettings;

			Nz::Vector3f planetDims = entityInstance.GetProperty<EntityPropertyType::FloatSize3D>("AtmospherePlanetDims");
			float planetCornerRadius = entityInstance.GetProperty<EntityPropertyType::Float>("CornerRadius");

			atmosphereScattering.type = AtmosphereScatteringType::RoundCube;
			atmosphereScattering.planetSettings = Nz::Vector4f(planetDims, planetCornerRadius);
		}
		else if (entityInstance.GetClass()->GetName() == "torus_planet")
		{
			Nz::Vector4f planetSettings;

			float planetRadius = entityInstance.GetProperty<EntityPropertyType::Float>("Radius");
			float planetThickness = entityInstance.GetProperty<EntityPropertyType::Float>("Thickness");

			atmosphereScattering.type = AtmosphereScatteringType::Torus;
			atmosphereScattering.planetSettings = Nz::Vector4f(planetRadius, planetThickness, 0.0f, 0.0f);
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
