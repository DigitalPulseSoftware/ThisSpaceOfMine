// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/EntityClass.hpp>
#include <fmt/format.h>

namespace tsom
{
	ClassInstanceComponent::ClassInstanceComponent(const EntityClass* entityClass) :
	m_entityClass(entityClass)
	{
		std::size_t propertyCount = entityClass->GetPropertyCount();

		m_properties.reserve(propertyCount);
		for (std::size_t i = 0; i < propertyCount; ++i)
			m_properties.emplace_back(entityClass->GetProperty(i).defaultValue);
	}

	Nz::UInt32 ClassInstanceComponent::FindPropertyIndex(std::string_view propertyName) const
	{
		return m_entityClass->FindProperty(propertyName);
	}

	Nz::UInt32 ClassInstanceComponent::GetPropertyIndex(std::string_view propertyName) const
	{
		Nz::UInt32 propertyIndex = FindPropertyIndex(propertyName);
		if (propertyIndex == EntityClass::InvalidIndex)
			throw std::runtime_error(fmt::format("invalid property {}", propertyName));

		return propertyIndex;
	}
}
