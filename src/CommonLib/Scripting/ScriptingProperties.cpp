// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/ScriptingProperties.hpp>
#include <sol/sol.hpp>

namespace tsom
{
	namespace
	{
		template<EntityPropertyType P>
		struct PropertyGetter
		{
			using UnderlyingType = EntityPropertyUnderlyingType_t<P>;

			UnderlyingType operator()(sol::object value)
			{
				return value.as<UnderlyingType>();
			}
		};

		template<EntityPropertyType P>
		struct PropertyPusher
		{
			template<typename T>
			sol::object operator()(sol::state_view& lua, T&& value)
			{
				return sol::make_object(lua, std::forward<T>(value));
			}
		};
	}

	EntityProperty TranslatePropertyFromLua(sol::object value, EntityPropertyType expectedType, bool isArray)
	{
		if (isArray)
		{
			sol::table content = value.as<sol::table>();
			std::size_t elementCount = content.size();

			auto HandleDataArray = [&](auto dummyType) -> EntityProperty
			{
				using T = std::decay_t<decltype(dummyType)>;

				EntityPropertyArrayValue<T::Property> container(elementCount);
				PropertyGetter<T::Property> getter;
				for (std::size_t i = 0; i < elementCount; ++i)
					container[i] = getter(content[i + 1]);

				return container;
			};

			switch (expectedType)
			{
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) case EntityPropertyType:: T: return HandleDataArray(EntityPropertyTag<EntityPropertyType:: T>{});

#include <CommonLib/EntityPropertyList.hpp>
			}
		}
		else
		{
			auto HandleData = [&](auto dummyType) -> EntityProperty
			{
				using T = std::decay_t<decltype(dummyType)>;

				PropertyGetter<T::Property> getter;
				return EntityPropertySingleValue<T::Property>{ getter(value) };
			};

			switch (expectedType)
			{
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) case EntityPropertyType:: T: return HandleData(EntityPropertyTag<EntityPropertyType:: T>{});

#include <CommonLib/EntityPropertyList.hpp>
			}
		}

		NAZARA_UNREACHABLE();
	}

	sol::object TranslatePropertyToLua(sol::state_view& lua, const EntityProperty& property)
	{
		return std::visit([&](auto&& value) -> sol::object
		{
			using T = std::decay_t<decltype(value)>;
			using PropertyTypeExtractor = EntityPropertyTypeExtractor<T>;
			constexpr bool IsArray = PropertyTypeExtractor::IsArray;

			PropertyPusher<PropertyTypeExtractor::Property> pusher;

			if constexpr (IsArray)
			{
				std::size_t elementCount = value.GetSize();
				sol::table content = lua.create_table(int(elementCount));

				for (std::size_t i = 0; i < elementCount; ++i)
					content[i + 1] = pusher(lua, value[i]);

				return content;
			}
			else
				return pusher(lua, *value);

		}, property);
	}
}
