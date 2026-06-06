// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Entities/ChunkClassLibrary.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Planets/RoundCubePlanet.hpp>
#include <CommonLib/Planets/TorusPlanet.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void ChunkClassLibrary::Register(EntityRegistry& registry)
	{
		registry.RegisterClass(EntityClass("round_cube_planet", {
			{
				EntityClass::Property { .name = "BlockSize",            .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                      .isNetworked = true },
				EntityClass::Property { .name = "CornerRadius",         .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(16.f),                     .isNetworked = true },
				EntityClass::Property { .name = "Gravity",              .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(9.81f),                    .isNetworked = true },
				EntityClass::Property { .name = "Seed",                 .type = EntityPropertyType::Integer,     .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(0),                      .isNetworked = true },
				EntityClass::Property { .name = "AtmospherePlanetDims", .type = EntityPropertyType::FloatSize3D, .defaultValue = EntityPropertySingleValue<EntityPropertyType::FloatSize3D>(Nz::Vector3f(60.f)), .isNetworked = true },
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

				InitializePlanetEntity(entity, std::make_shared<RoundCubePlanet>(m_app, blockSize, Nz::SafeCaster(seed), gravity, cornerRadius));
			}
		},
		{}));

		registry.RegisterClass(EntityClass("torus_planet", {
			{
				EntityClass::Property { .name = "BlockSize",            .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                      .isNetworked = true },
				EntityClass::Property { .name = "Radius",               .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(80.f),                     .isNetworked = true },
				EntityClass::Property { .name = "Thickness",            .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(20.f),                     .isNetworked = true },
				EntityClass::Property { .name = "Gravity",              .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(9.81f),                    .isNetworked = true },
				EntityClass::Property { .name = "Seed",                 .type = EntityPropertyType::Integer,     .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(0),                      .isNetworked = true },
				EntityClass::Property { .name = "AtmospherePlanetDims", .type = EntityPropertyType::FloatSize3D, .defaultValue = EntityPropertySingleValue<EntityPropertyType::FloatSize3D>(Nz::Vector3f(60.f)), .isNetworked = true },
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

				InitializePlanetEntity(entity, std::make_shared<TorusPlanet>(m_app, blockSize, Nz::SafeCaster(seed), gravity, radius, thickness));
			}
		},
		{}));

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

				InitializeShipEntity(entity, std::make_unique<Ship>(cellSize));
			}
		},
		{}));
	}

	PlanetComponent& ChunkClassLibrary::InitializePlanetEntity(entt::handle entity, std::shared_ptr<Planet>&& planet)
	{
		auto& planetComponent = entity.emplace<PlanetComponent>();
		planetComponent.planet = std::move(planet);

		Nz::EnttWorld* world = entity.registry()->ctx().get<Nz::EnttWorld*>();
		for (std::size_t layerIndex = 0; layerIndex < planetComponent.planetEntities.size(); ++layerIndex)
		{
			if (!m_blockLibrary.IsValidLayer(layerIndex))
				continue;

			planetComponent.planetEntities[layerIndex] = SetupChunkEntities(*world, *planetComponent.planet, layerIndex);
			planetComponent.planetEntities[layerIndex]->SetParentEntity(entity);
		}

		return planetComponent;
	}

	ShipComponent& ChunkClassLibrary::InitializeShipEntity(entt::handle entity, std::unique_ptr<Ship>&& ship)
	{
		auto& shipComponent = entity.emplace<ShipComponent>();
		shipComponent.ship = std::move(ship);

		Nz::EnttWorld* world = entity.registry()->ctx().get<Nz::EnttWorld*>();
		for (std::size_t layerIndex = 0; layerIndex < shipComponent.shipEntities.size(); ++layerIndex)
		{
			if (!m_blockLibrary.IsValidLayer(layerIndex))
				continue;

			shipComponent.shipEntities[layerIndex] = SetupChunkEntities(*world, *shipComponent.ship, layerIndex);
			shipComponent.shipEntities[layerIndex]->SetParentEntity(entity);
		}

		return shipComponent;
	}

	std::unique_ptr<ChunkEntities> ChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer, std::size_t layerIndex)
	{
		return std::make_unique<ChunkEntities>(m_app, world, chunkContainer, m_blockLibrary, layerIndex);
	}
}
