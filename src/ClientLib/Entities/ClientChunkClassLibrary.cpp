// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/Components/ChunkLODComponent.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <CommonLib/Asteroid.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Components/ChunkContainerComponent.hpp>
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

	void ClientChunkClassLibrary::InitializeAsteroidEntity(entt::handle entity, std::shared_ptr<Asteroid>&& asteroid)
	{
		float radius = asteroid->GetRadius();

		ChunkClassLibrary::InitializeAsteroidEntity(entity, std::move(asteroid));

		ChunkContainerComponent& chunkContainer = entity.get<ChunkContainerComponent>();

		// Setup LOD
		ChunkLODComponent& chunkLod = entity.emplace<ChunkLODComponent>();
		chunkLod.radius = radius;

		std::array<float, 2> lodFactors = { 0.02f, 0.005f };
		static_assert(lodFactors.size() <= Constants::MaxChunkLevelOfDetail);

		for (std::size_t lod = 0; lod < lodFactors.size(); ++lod)
		{
			const ChunkContainer& parentContainer = (lod == 0) ? *chunkContainer.chunkContainer : *chunkLod.chunkContainerLOD[lod - 1].chunkContainer;
			chunkLod.chunkContainerLOD.push_back({
				.chunkContainer = std::make_shared<ChunkContainerLOD>(parentContainer),
				.screenSize = lodFactors[lod]
			});
		}

		chunkLod.chunkContainerLOD.push_back({
			.chunkContainer = nullptr, //< disable rendering
			.screenSize = 0.001f
		});
	}

	ChunkContainerComponent& ClientChunkClassLibrary::InitializeChunkEntity(entt::handle entity, bool isStatic, std::shared_ptr<ChunkContainer>&& chunkContainer)
	{
		entity.emplace<ChunkNetworkMapComponent>();

		return ChunkClassLibrary::InitializeChunkEntity(entity, isStatic, std::move(chunkContainer));
	}

	void ClientChunkClassLibrary::InitializePlanetEntity(entt::handle entity, std::shared_ptr<Planet>&& planet)
	{
		float radius = planet->GetRadius();

		ChunkClassLibrary::InitializePlanetEntity(entity, std::move(planet));

		ChunkContainerComponent& chunkContainer = entity.get<ChunkContainerComponent>();

		// Setup LOD
		ChunkLODComponent& chunkLod = entity.emplace<ChunkLODComponent>();
		chunkLod.radius = radius;

		std::array<float, 4> lodFactors = { 0.1f, 0.04f, 0.02f, 0.015f };
		static_assert(lodFactors.size() <= Constants::MaxChunkLevelOfDetail);

		for (std::size_t lod = 0; lod < lodFactors.size(); ++lod)
		{
			const ChunkContainer& parentContainer = (lod == 0) ? *chunkContainer.chunkContainer : *chunkLod.chunkContainerLOD[lod - 1].chunkContainer;
			chunkLod.chunkContainerLOD.push_back({
				.chunkContainer = std::make_shared<ChunkContainerLOD>(parentContainer),
				.screenSize = lodFactors[lod]
			});
		}

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
	}

	std::unique_ptr<ChunkEntities> ClientChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, std::shared_ptr<ChunkContainer> chunkContainer, std::size_t layerIndex, bool isStatic)
	{
		return std::make_unique<ClientChunkEntities>(m_app, m_config, world, std::move(chunkContainer), Nz::SafeCast<const ClientBlockLibrary&>(m_blockLibrary), layerIndex, isStatic);
	}
}
