// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cassert>

namespace tsom
{
	template<EntityPropertyType Property>
	auto ClassInstanceComponent::GetProperty(std::size_t propertyIndex) const
	{
		assert(propertyIndex < properties.size());
		return std::get<EntityPropertySingleValue<Property>>(properties[propertyIndex]);
	}

	template<EntityPropertyType Property>
	auto ClassInstanceComponent::GetProperty(std::string_view propertyName) const
	{
		return GetProperty<Property>(GetPropertyIndex(propertyName));
	}

	template<EntityPropertyType Property, typename T>
	void ClassInstanceComponent::UpdateProperty(std::size_t propertyIndex, T&& value)
	{
		assert(propertyIndex < properties.size());
		std::get<EntityPropertySingleValue<Property>>(properties[propertyIndex]) = std::forward<T>(value);
	}

	template<EntityPropertyType Property, typename T>
	void ClassInstanceComponent::UpdateProperty(std::string_view propertyName, T&& value)
	{
		return UpdateProperty<Property>(GetPropertyIndex(propertyName), std::forward<T>(value));
	}
}
