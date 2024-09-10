// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_CLASSINSTANCECOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_CLASSINSTANCECOMPONENT_HPP

#include <CommonLib/EntityProperties.hpp>
#include <string_view>
#include <vector>

namespace tsom
{
	class EntityClass;

	struct TSOM_COMMONLIB_API ClassInstanceComponent
	{
		std::vector<EntityProperty> properties;
		const EntityClass* entityClass;

		template<EntityPropertyType Property> auto GetProperty(std::size_t propertyIndex) const;
		template<EntityPropertyType Property> auto GetProperty(std::string_view propertyName) const;

		Nz::UInt32 GetPropertyIndex(std::string_view propertyName) const;

		template<EntityPropertyType Property, typename T> void UpdateProperty(std::size_t propertyIndex, T&& value);
		template<EntityPropertyType Property, typename T> void UpdateProperty(std::string_view propertyName, T&& value);
	};
}

#include <CommonLib/Components/ClassInstanceComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_CLASSINSTANCECOMPONENT_HPP
