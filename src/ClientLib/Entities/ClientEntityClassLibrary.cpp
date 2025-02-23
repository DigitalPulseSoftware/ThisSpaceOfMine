// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Entities/ClientEntityClassLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>

namespace tsom
{
	void ClientEntityClassLibrary::OnPlayerActivate(entt::handle entity)
	{
		auto& entityInstance = entity.get<ClassInstanceComponent>();

		// TODO: Cache those
		Nz::UInt32 healthIndex = m_playerClass->FindProperty("health");
		Nz::UInt32 oxygenIndex = m_playerClass->FindProperty("oxygen");

		entityInstance.OnPropertyUpdate.Connect([entity, healthIndex, oxygenIndex](ClassInstanceComponent* classInstance, Nz::UInt32 propertyIndex, const EntityProperty& newValue) mutable
		{
			if (propertyIndex == healthIndex)
			{
				auto& healthValue = std::get<EntityPropertySingleValue<EntityPropertyType::Integer>>(newValue);
				fmt::print("Health value: {}\n", *healthValue);
			}
			else if (propertyIndex == oxygenIndex)
			{
				auto& oxygenValue = std::get<EntityPropertySingleValue<EntityPropertyType::Integer>>(newValue);
				fmt::print("Oxygen value: {}\n", *oxygenValue);
			}
		});
	}

	void ClientEntityClassLibrary::OnPlayerRpc_Death(entt::handle entity)
	{
		fmt::print("YOU DIED\n");
	}
}
