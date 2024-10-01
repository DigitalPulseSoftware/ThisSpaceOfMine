// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cassert>

namespace tsom
{
	inline const std::shared_ptr<const EntityClass>& ClassInstanceComponent::GetClass() const
	{
		return m_entityClass;
	}

	inline const EntityProperty& ClassInstanceComponent::GetProperty(Nz::UInt32 propertyIndex) const
	{
		assert(propertyIndex < m_properties.size());
		return m_properties[propertyIndex];
	}

	template<EntityPropertyType Property>
	auto ClassInstanceComponent::GetProperty(Nz::UInt32 propertyIndex) const
	{
		assert(propertyIndex < m_properties.size());
		return std::get<EntityPropertySingleValue<Property>>(m_properties[propertyIndex]);
	}

	template<EntityPropertyType Property>
	auto ClassInstanceComponent::GetProperty(std::string_view propertyName) const
	{
		return GetProperty<Property>(GetPropertyIndex(propertyName));
	}

	inline void ClassInstanceComponent::UpdateProperty(Nz::UInt32 propertyIndex, EntityProperty&& value)
	{
		assert(propertyIndex < m_properties.size());
		OnPropertyUpdate(this, propertyIndex, value);
		m_properties[propertyIndex] = std::move(value);
	}

	template<EntityPropertyType Property, typename T>
	void ClassInstanceComponent::UpdateProperty(Nz::UInt32 propertyIndex, T&& value)
	{
		return UpdateProperty(propertyIndex, EntityPropertySingleValue<Property>(std::forward<T>(value)));
	}

	template<EntityPropertyType Property, typename T>
	void ClassInstanceComponent::UpdateProperty(std::string_view propertyName, T&& value)
	{
		return UpdateProperty<Property>(GetPropertyIndex(propertyName), std::forward<T>(value));
	}
}
