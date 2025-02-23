// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Entities/SharedEntityClassLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/Components/BuoyancyComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	SharedEntityClassLibrary::SharedEntityClassLibrary(Nz::ApplicationBase& app) :
	m_app(app)
	{
		m_playerCollider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);
	}

	void SharedEntityClassLibrary::Register(EntityRegistry& registry)
	{
		m_playerClass = registry.RegisterClass(EntityClass("player",
		{
			EntityClass::Property { .name = "health", .type = EntityPropertyType::Integer, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(100), .isArray = false, .isNetworked = true },
			EntityClass::Property { .name = "oxygen", .type = EntityPropertyType::Integer, .defaultValue = EntityPropertySingleValue<EntityPropertyType::Integer>(100), .isArray = false, .isNetworked = true },
		},
		{
			.onActivate = [this](entt::handle entity)
			{
				OnPlayerActivate(entity);
			},
			.onInit = [this](entt::handle entity)
			{
				OnPlayerInit(entity);
			}
		},
		{
			EntityClass::RemoteProcedureCall
			{
				.onCalled = [this](entt::handle entity)
				{
					OnPlayerRpc_Death(entity);
				},
				.name = "death"
			}
		}));

		m_shipExteriorClass = registry.RegisterClass(EntityClass("ship_exterior", {},
		{
			.onActivate = [this](entt::handle entity)
			{
				OnShipExteriorActivate(entity);
			},
			.onInit = [this](entt::handle entity)
			{
				OnShipExteriorInit(entity);
			}
		},
		{
		}));
	}

	void SharedEntityClassLibrary::OnPlayerActivate(entt::handle entity)
	{
		entity.emplace<BuoyancyComponent>();
	}

	void SharedEntityClassLibrary::OnPlayerInit(entt::handle entity)
	{
	}

	void SharedEntityClassLibrary::OnPlayerRpc_Death(entt::handle entity)
	{
	}

	void SharedEntityClassLibrary::OnShipExteriorActivate(entt::handle entity)
	{
	}

	void SharedEntityClassLibrary::OnShipExteriorInit(entt::handle entity)
	{
	}
}
