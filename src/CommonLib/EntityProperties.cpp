// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/Protocol/CompressedInteger.hpp>
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

namespace Nz
{
	bool Deserialize(SerializationContext& context, tsom::EntityProperty* entityProperty, TypeTag<tsom::EntityProperty>)
	{
		Nz::UInt8 propertyTypeInt;
		if (!Deserialize(context, &propertyTypeInt))
			return false;

		tsom::EntityPropertyType propertyType = static_cast<tsom::EntityPropertyType>(propertyTypeInt);

		Nz::UInt8 isArrayInt;
		if (!Deserialize(context, &isArrayInt))
			return false;

		bool isArray = (isArrayInt != 0);

		auto Unserialize = [&](auto dummyType)
		{
			using T = std::decay_t<decltype(dummyType)>;

			static constexpr tsom::EntityPropertyType Property = T::Property;

			if (isArray)
			{
				tsom::CompressedUnsigned<Nz::UInt32> size;
				if (!Deserialize(context, &size))
					return false;

				auto& elements = entityProperty->emplace<tsom::EntityPropertyArrayValue<Property>>(size);
				for (auto& element : elements)
				{
					if (!Deserialize(context, &element))
						return false;
				}
			}
			else
			{
				auto& value = entityProperty->emplace<tsom::EntityPropertySingleValue<Property>>();
				if (!Deserialize(context, &value.value))
					return false;
			}

			return true;
		};

		switch (propertyType)
		{
#define TSOM_ENTITYPROPERTYTYPE(V, T, UT) case tsom::EntityPropertyType:: T: return Unserialize(tsom::EntityPropertyTag<tsom::EntityPropertyType:: T>{}); break;

#include <CommonLib/EntityPropertyList.hpp>
		}

		NAZARA_UNREACHABLE();
	}

	bool Serialize(SerializationContext& context, const tsom::EntityProperty& entityProperty, TypeTag<tsom::EntityProperty>)
	{
		auto [internalType, isArray] = tsom::ExtractPropertyType(entityProperty);

		Nz::UInt8 propertyType = Nz::UInt8(internalType);
		if (!Serialize(context, propertyType))
			return false;

		if (!Serialize(context, Nz::UInt8((isArray) ? 1 : 0)))
			return false;

		return std::visit([&](auto&& propertyValue)
		{
			using T = std::decay_t<decltype(propertyValue)>;
			using TypeExtractor = tsom::EntityPropertyTypeExtractor<T>;
			constexpr bool IsArray = TypeExtractor::IsArray;

			if constexpr (IsArray)
			{
				tsom::CompressedUnsigned<UInt32> arraySize(SafeCaster(propertyValue.size()));
				if (!Serialize(context, arraySize))
					return false;

				for (auto& element : propertyValue)
				{
					if (!Serialize(context, element))
						return false;
				}
			}
			else
			{
				if (!Serialize(context, propertyValue.value))
					return false;
			}

			return true;
		}, entityProperty);
	}
}
