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
#include <entt/entt.hpp>

namespace tsom
{
	void ChunkClassLibrary::Register(EntityRegistry& registry)
	{
		registry.RegisterClass(EntityClass("planet", {
			{
				EntityClass::Property { .name = "CellSize",             .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),                      .isNetworked = true },
				EntityClass::Property { .name = "CornerRadius",         .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(16.f),                     .isNetworked = true },
				EntityClass::Property { .name = "Gravity",              .type = EntityPropertyType::Float,       .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(9.81f),                    .isNetworked = true },
				EntityClass::Property { .name = "AtmospherePlanetDims", .type = EntityPropertyType::FloatSize3D, .defaultValue = EntityPropertySingleValue<EntityPropertyType::FloatSize3D>(Nz::Vector3f(60.f)), .isNetworked = true },
			}
		},
		{
			.onInit = [this](entt::handle entity)
			{
				auto& entityInstance = entity.get<ClassInstanceComponent>();

				float cellSize = entityInstance.GetProperty<EntityPropertyType::Float>(0);
				float cornerRadius = entityInstance.GetProperty<EntityPropertyType::Float>(1);
				float gravity = entityInstance.GetProperty<EntityPropertyType::Float>(2);

				Nz::EnttWorld* world = entity.registry()->ctx().get<Nz::EnttWorld*>();

				auto& planetComponent = entity.emplace<PlanetComponent>();
				planetComponent.planet = std::make_shared<Planet>(m_app, cellSize, cornerRadius, gravity);
				for (std::size_t layerIndex = 0; layerIndex < planetComponent.planetEntities.size(); ++layerIndex)
				{
					if (!m_blockLibrary.IsValidLayer(layerIndex))
						continue;

					planetComponent.planetEntities[layerIndex] = SetupChunkEntities(*world, entity, *planetComponent.planet, layerIndex);
				}

				InitializePlanetEntity(entity);
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

				Nz::EnttWorld* world = entity.registry()->ctx().get<Nz::EnttWorld*>();

				auto& shipComponent = entity.emplace<ShipComponent>();
				shipComponent.ship = std::make_unique<Ship>(cellSize);
				for (std::size_t layerIndex = 0; layerIndex < shipComponent.shipEntities.size(); ++layerIndex)
				{
					if (!m_blockLibrary.IsValidLayer(layerIndex))
						continue;

					shipComponent.shipEntities[layerIndex] = SetupChunkEntities(*world, entity, *shipComponent.ship, layerIndex);
				}

				InitializeShipEntity(entity);
			}
		},
		{}));
	}

	void ChunkClassLibrary::InitializePlanetEntity(entt::handle entity)
	{
	}

	void ChunkClassLibrary::InitializeShipEntity(entt::handle entity)
	{
	}

	std::shared_ptr<ChunkEntities> ChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, entt::handle parentEntity, ChunkContainer& chunkContainer, std::size_t layerIndex)
	{
		return std::make_shared<ChunkEntities>(m_app, world, parentEntity, chunkContainer, m_blockLibrary, layerIndex);
	}
}
