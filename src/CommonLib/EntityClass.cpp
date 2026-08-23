// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Utility/JsonSerialization.hpp>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace tsom
{
	EntityClass::EntityClass(std::string name, std::vector<Property> properties, Callbacks callbacks, std::vector<RemoteProcedureCall> clientRpcs, std::vector<RemoteProcedureCall> serverRpcs, Nz::ParameterList metadata) :
	m_name(std::move(name)),
	m_properties(std::move(properties)),
	m_clientRpcs(std::move(clientRpcs)),
	m_serverRpcs(std::move(serverRpcs)),
	m_metadata(std::move(metadata)),
	m_callbacks(std::move(callbacks))
	{
		for (const auto& clientRpc : m_clientRpcs)
		{
			if (FindClientRpc(clientRpc.name) != InvalidIndex)
				throw std::runtime_error(fmt::format("client rpc {} already exists", clientRpc.name));

			m_clientRpcIndices.emplace(clientRpc.name, m_clientRpcIndices.size());
		}

		for (const auto& property : m_properties)
		{
			if (FindProperty(property.name) != InvalidIndex)
				throw std::runtime_error(fmt::format("property {} already exists", property.name));

			m_propertyIndices.emplace(property.name, m_propertyIndices.size());
		}
	}

	void EntityClass::ActivateEntity(entt::handle entity) const
	{
		NazaraAssert(entity.get<ClassInstanceComponent>().GetClass().get() == this);
		if (m_callbacks.onActivate)
			m_callbacks.onActivate(entity);
	}

	void EntityClass::DestroyEntity(entt::handle entity) const
	{
		NazaraAssert(entity.get<ClassInstanceComponent>().GetClass().get() == this);
		if (m_callbacks.onDestroy)
			m_callbacks.onDestroy(entity);
	}

	void EntityClass::InitAndActivateEntity(entt::handle entity) const
	{
		NazaraAssert(entity.get<ClassInstanceComponent>().GetClass().get() == this);
		if (m_callbacks.onInit)
			m_callbacks.onInit(entity);

		if (m_callbacks.onActivate)
			m_callbacks.onActivate(entity);
	}

	void EntityClass::InitEntity(entt::handle entity) const
	{
		NazaraAssert(entity.get<ClassInstanceComponent>().GetClass().get() == this);
		if (m_callbacks.onInit)
			m_callbacks.onInit(entity);
	}

	std::vector<EntityProperty> EntityClass::PropertiesFromJson(const nlohmann::json& propertiesJson) const
	{
		std::vector<EntityProperty> properties;
		properties.reserve(m_properties.size());
		for (const auto& property : m_properties)
			properties.push_back(property.defaultValue);

		PropertiesFromJson(properties, propertiesJson);

		return properties;
	}

	void EntityClass::PropertiesFromJson(std::vector<EntityProperty>& properties, const nlohmann::json& propertiesJson, std::string_view prefix) const
	{
		for (auto&& [key, value] : propertiesJson.items())
		{
			std::string propertyName = fmt::format("{}{}", prefix, key);

			auto it = m_propertyIndices.find(propertyName);
			if (it == m_propertyIndices.end())
			{
				if (value.is_object())
				{
					// Handle unknown objects as sub-properties
					PropertiesFromJson(properties, value, fmt::format("{}.", key));
					continue;
				}

				spdlog::warn("property {} from json doesn't exist in class {} property list", key, m_name);
				continue;
			}

			std::size_t propertyIndex = it->second;
			const auto& propertyData = m_properties[propertyIndex];

			auto Deserialize = [&, &value = value](auto dummyType)
			{
				using T = std::decay_t<decltype(dummyType)>;

				static constexpr EntityPropertyType Property = T::Property;
				using UnderlyingType = EntityPropertyUnderlyingType_t<Property>;

				if (propertyData.isArray)
				{
					if (!value.is_array())
						throw std::runtime_error("expected array");

					std::size_t elementCount = value.size();
					if (elementCount == 0)
						return; //< Ignore empty arrays

					EntityPropertyArrayValue<Property> elements(elementCount);
					for (std::size_t i = 0; i < elementCount; ++i)
						elements[i] = value[i];

					properties[propertyIndex] = std::move(elements);
				}
				else
					properties[propertyIndex] = EntityPropertySingleValue<Property>(value.get<UnderlyingType>());
			};

			switch (propertyData.type)
			{
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) case EntityPropertyType:: T: Deserialize(EntityPropertyTag<EntityPropertyType:: T>{}); break;

#include <CommonLib/EntityPropertyList.hpp>
			}
		}
	}

	nlohmann::json EntityClass::PropertiesToJson(std::span<const EntityProperty> properties) const
	{
		auto propertiesObject = nlohmann::json::object();
		for (auto&& [propertyName, propertyIndex] : m_propertyIndices)
		{
			NazaraAssert(propertyIndex < properties.size());

			std::visit([&, &propertyName = propertyName](auto&& propertyValue)
			{
				using T = std::decay_t<decltype(propertyValue)>;
				using TypeExtractor = EntityPropertyTypeExtractor<T>;
				constexpr bool IsArray = TypeExtractor::IsArray;

				nlohmann::json* value = &propertiesObject;
				Nz::SplitString(propertyName, ".", [&](std::string_view part)
				{
					value = &(*value)[part];
					return true;
				});

				if constexpr (IsArray)
				{
					auto elementArray = nlohmann::json::array();
					for (std::size_t i = 0; i < propertyValue.size(); ++i)
						elementArray.push_back(propertyValue[i]);

					*value = std::move(elementArray);
				}
				else
					*value = propertyValue.value;

			}, properties[propertyIndex]);
		}

		return propertiesObject;
	}
}
