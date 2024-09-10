// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>
#include <stdexcept>

namespace tsom
{
	EntityClass::EntityClass(std::string name, std::vector<Property> properties, Callbacks callbacks) :
	m_name(std::move(name)),
	m_properties(std::move(properties)),
	m_callbacks(std::move(callbacks))
	{
		for (const auto& property : m_properties)
		{
			if (FindProperty(property.name) != InvalidIndex)
				throw std::runtime_error(fmt::format("property {} already exists", name));

			m_propertyIndices.emplace(property.name, m_propertyIndices.size());
		}
	}

	void EntityClass::ActivateEntity(entt::handle entity) const
	{
		assert(entity.get<ClassInstanceComponent>().entityClass == this);
		if (m_callbacks.onInit)
			m_callbacks.onInit(entity);
	}

	ClassInstanceComponent& EntityClass::SetupEntity(entt::handle entity) const
	{
		auto& entityInstance = entity.emplace<ClassInstanceComponent>();
		entityInstance.entityClass = this;
		entityInstance.properties.reserve(m_properties.size());
		for (const auto& property : m_properties)
			entityInstance.properties.emplace_back(property.defaultValue);

		return entityInstance;
	}
}