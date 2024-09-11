// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Entities/ChunkClassLibrary.hpp>
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
				EntityClass::Property { .name = "CellSize",     .type = EntityPropertyType::Float, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(1.f),   .isNetworked = true },
				EntityClass::Property { .name = "CornerRadius", .type = EntityPropertyType::Float, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(16.f),  .isNetworked = true },
				EntityClass::Property { .name = "Gravity",      .type = EntityPropertyType::Float, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Float>(9.81f), .isNetworked = true }
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
				planetComponent.planet = std::make_unique<Planet>(cellSize, cornerRadius, gravity);
				planetComponent.planetEntities = SetupChunkEntities(*world, *planetComponent.planet);
				planetComponent.planetEntities->SetParentEntity(entity);

				InitializeChunkEntity(entity);
			}
		}));

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
				shipComponent.shipEntities = SetupChunkEntities(*world, *shipComponent.ship);
				shipComponent.shipEntities->SetParentEntity(entity);

				InitializeChunkEntity(entity);
			}
		}));
	}

	void ChunkClassLibrary::InitializeChunkEntity(entt::handle entity)
	{
	}

	std::unique_ptr<ChunkEntities> ChunkClassLibrary::SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer)
	{
		return std::make_unique<ChunkEntities>(m_app, world, chunkContainer, m_blockLibrary);
	}
}
