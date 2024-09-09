// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityProperties.hpp>
#include <fmt/format.h>

namespace tsom
{
	std::pair<EntityPropertyType, bool> ExtractPropertyType(const EntityProperty& value)
	{
		return std::visit([&](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			using TypeExtractor = EntityPropertyTypeExtractor<T>;

			return std::make_pair(TypeExtractor::Property, TypeExtractor::IsArray);
		}, value);
	}

	EntityPropertyType ParseEntityPropertyType(std::string_view str)
	{
		auto CompareStringCaseInsensitive = [](std::string_view s1, std::string_view s2)
		{
			return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(), [](char c1, char c2)
			{
				return std::tolower(c1) == std::tolower(c2);
			});
		};

#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) if (CompareStringCaseInsensitive(str, #T)) return EntityPropertyType:: T;

#include <CommonLib/EntityPropertyList.hpp>

		throw std::runtime_error(fmt::format("invalid property type {}", str));
	}

	std::string_view ToString(EntityPropertyType propertyType)
	{
		switch (propertyType)
		{
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) case EntityPropertyType:: T: return #T ;

#include <CommonLib/EntityPropertyList.hpp>
		}

		NAZARA_UNREACHABLE();
	}
}
