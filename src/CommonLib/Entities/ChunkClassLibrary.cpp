// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Entities/ChunkClassLibrary.hpp>
#include <CommonLib/Asteroid.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ChunkContainerComponent.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Planets/RoundCubePlanet.hpp>
#include <CommonLib/Planets/TorusPlanet.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void ChunkClassLibrary::Register(EntityRegistry& registry)
	{
		registry.RegisterClass(EntityClass("round_cube_planet", {
			{
				EntityClass::Property { .name = "BlockSize",                     .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                                     .isNetworked = true },
				EntityClass::Property { .name = "CornerRadius",                  .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(16.f),                                    .isNetworked = true },
				EntityClass::Property { .name = "Gravity",                       .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(9.81f),                                   .isNetworked = true },
				EntityClass::Property { .name = "Seed",                          .type = EntityPropertyType::Integer,     .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(0),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.Shape",              .type = EntityPropertyType::String,      .defaultValue = EntityPropertySingleValue<EntityPropertyType::String>("RoundCube"),                            .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.ShapeSettings",      .type = EntityPropertyType::Float4,      .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float4>(Nz::Vector4f(60.f, 60.f, 60.f, 16.f)),   .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.MaxHeight",          .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(192.f),                                   .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.ScatteringStrength", .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.WaveLengths",        .type = EntityPropertyType::Float3,      .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float3>(Nz::Vector3f(700.f, 530.f, 440.f)),      .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.MieScattering",      .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(0.9),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.MieHeight",          .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(5.f),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.DensityFalloff",     .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(20.f),                                    .isNetworked = true },
			}
		},
		{
			.onInit = [this](entt::handle entity)
			{
				auto& entityInstance = entity.get<ClassInstanceComponent>();

				float blockSize = entityInstance.GetProperty<EntityPropertyType::Float>(0);
				float cornerRadius = entityInstance.GetProperty<EntityPropertyType::Float>(1);
				float gravity = entityInstance.GetProperty<EntityPropertyType::Float>(2);
				Nz::Int64 seed = entityInstance.GetProperty<EntityPropertyType::Integer>(3);

				InitializePlanetEntity(entity, std::make_shared<RoundCubePlanet>(m_app, m_blockLibrary, blockSize, Nz::SafeCaster(seed), gravity, cornerRadius));
			}
		},
		{}, {}));

		registry.RegisterClass(EntityClass("torus_planet", {
			{
				EntityClass::Property { .name = "BlockSize",                     .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                                     .isNetworked = true },
				EntityClass::Property { .name = "Radius",                        .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(80.f),                                    .isNetworked = true },
				EntityClass::Property { .name = "Thickness",                     .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(20.f),                                    .isNetworked = true },
				EntityClass::Property { .name = "Gravity",                       .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(9.81f),                                   .isNetworked = true },
				EntityClass::Property { .name = "Seed",                          .type = EntityPropertyType::Integer,     .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(0),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.Shape",              .type = EntityPropertyType::String,      .defaultValue = EntityPropertySingleValue<EntityPropertyType::String>("RoundCube"),                            .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.ShapeSettings",      .type = EntityPropertyType::Float4,      .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float4>(Nz::Vector4f(60.f, 60.f, 60.f, 16.f)),   .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.MaxHeight",          .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(192.f),                                   .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.ScatteringStrength", .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.WaveLengths",        .type = EntityPropertyType::Float3,      .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float3>(Nz::Vector3f(700.f, 530.f, 440.f)),      .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.MieScattering",      .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(0.9),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.MieHeight",          .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(5.f),                                     .isNetworked = true },
				EntityClass::Property { .name = "Atmosphere.DensityFalloff",     .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(20.f),                                    .isNetworked = true },
			}
		},
		{
			.onInit = [this](entt::handle entity)
			{
				auto& entityInstance = entity.get<ClassInstanceComponent>();

				float blockSize = entityInstance.GetProperty<EntityPropertyType::Float>(0);
				float radius = entityInstance.GetProperty<EntityPropertyType::Float>(1);
				float thickness = entityInstance.GetProperty<EntityPropertyType::Float>(2);
				float gravity = entityInstance.GetProperty<EntityPropertyType::Float>(3);
				Nz::Int64 seed = entityInstance.GetProperty<EntityPropertyType::Integer>(4);

				InitializePlanetEntity(entity, std::make_shared<TorusPlanet>(m_app, m_blockLibrary, blockSize, Nz::SafeCaster(seed), gravity, radius, thickness));
			}
		},
		{}, {}));

		registry.RegisterClass(EntityClass("ship", {
			{
				EntityClass::Property { .name = "CellSize", .type = EntityPropertyType::Float, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f), .isNetworked = true }
			}
		},
		{
			.onInit = [this](entt::handle entity)
			{
				auto& entityInstance = entity.get<ClassInstanceComponent>();

				float cellSize = entityInstance.GetProperty<EntityPropertyType::Float>(0);

				InitializeShipEntity(entity, std::make_shared<Ship>(m_blockLibrary, cellSize));
			}
		},
		{}, {}));

		registry.RegisterClass(EntityClass("asteroid", {
			{
				EntityClass::Property { .name = "BlockSize", .type = EntityPropertyType::Float,   .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(0.5f), .isNetworked = true },
				EntityClass::Property { .name = "Radius",    .type = EntityPropertyType::Float,   .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(8.f),  .isNetworked = true },
				EntityClass::Property { .name = "Seed",      .type = EntityPropertyType::Integer, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(0),  .isNetworked = true }
			}
		},
		{
			.onInit = [this](entt::handle entity)
			{
				auto& entityInstance = entity.get<ClassInstanceComponent>();

				float cellSize = entityInstance.GetProperty<EntityPropertyType::Float>(0);
				float radius = entityInstance.GetProperty<EntityPropertyType::Float>(1);
				Nz::Int64 seed = entityInstance.GetProperty<EntityPropertyType::Integer>(2);

				InitializeAsteroidEntity(entity, std::make_shared<Asteroid>(m_app, m_blockLibrary, cellSize, radius, Nz::SafeCaster(seed)));
			}
		},
		{}, {}));
	}

	void ChunkClassLibrary::InitializeAsteroidEntity(entt::handle entity, std::shared_ptr<Asteroid>&& asteroid)
	{
		Nz::RigidBody3D::DynamicSettings physicsSettings;
		physicsSettings.objectLayer = Constants::ObjectLayerDynamic;
		physicsSettings.mass = 1000.0f;

		entity.emplace<Nz::RigidBody3DComponent>(physicsSettings);

		InitializeChunkEntity(entity, false, std::move(asteroid));
	}

	ChunkContainerComponent& ChunkClassLibrary::InitializeChunkEntity(entt::handle entity, bool isStatic, std::shared_ptr<ChunkContainer>&& chunkContainer)
	{
		auto& chunkContainerComponent = entity.emplace<ChunkContainerComponent>();
		chunkContainerComponent.chunkContainer = std::move(chunkContainer);

		Nz::EnttWorld* world = entity.registry()->ctx().get<Nz::EnttWorld*>();
		for (std::size_t layerIndex = 0; layerIndex < chunkContainerComponent.chunkEntities.size(); ++layerIndex)
		{
			if (!m_blockLibrary.IsValidLayer(layerIndex))
				continue;

			chunkContainerComponent.chunkEntities[layerIndex] = SetupChunkEntities(*world, chunkContainerComponent.chunkContainer, layerIndex, isStatic);
			chunkContainerComponent.chunkEntities[layerIndex]->SetParentEntity(entity);
		}

		return chunkContainerComponent;
	}

	void ChunkClassLibrary::InitializePlanetEntity(entt::handle entity, std::shared_ptr<Planet>&& planet)
	{
		entity.emplace<PlanetComponent>().planet = planet;

		InitializeChunkEntity(entity, true, std::move(planet));
	}

	void ChunkClassLibrary::InitializeShipEntity(entt::handle entity, std::shared_ptr<Ship>&& ship)
	{
		entity.emplace<ShipComponent>().ship = ship;

		InitializeChunkEntity(entity, true, std::move(ship)); //< Ship chunk entities are static in their own environment, it's the ship environment that's moving!
	}
}
