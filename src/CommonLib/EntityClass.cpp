// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Utility/JsonSerialization.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace tsom
{
	EntityClass::EntityClass(std::string name, std::vector<Property> properties, Callbacks callbacks, std::vector<RemoteProcedureCall> clientRpcs) :
	m_name(std::move(name)),
	m_properties(std::move(properties)),
	m_clientRpcs(std::move(clientRpcs)),
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

		for (auto&& [propertyName, propertyValue] : propertiesJson.items())
		{
			auto it = m_propertyIndices.find(propertyName);
			if (it == m_propertyIndices.end())
			{
				NazaraWarning("property {} from json doesn't exist in class property list", propertyName);
				continue;
			}

			std::size_t propertyIndex = it->second;
			const auto& propertyData = m_properties[propertyIndex];

			auto Deserialize = [&, &propertyName = propertyName, &propertyValue = propertyValue](auto dummyType)
			{
				using T = std::decay_t<decltype(dummyType)>;

				static constexpr EntityPropertyType Property = T::Property;
				using UnderlyingType = EntityPropertyUnderlyingType_t<Property>;

				if (propertyData.isArray)
				{
					if (!propertyValue.is_array())
						throw std::runtime_error("Expected array");

					std::size_t elementCount = propertyValue.size();
					if (elementCount == 0)
						return; //< Ignore empty arrays

					EntityPropertyArrayValue<Property> elements(elementCount);
					for (std::size_t i = 0; i < elementCount; ++i)
						elements[i] = propertyValue[i];

					properties[propertyIndex] = std::move(elements);
				}
				else
				{
					UnderlyingType extractedValue = propertyValue;
					EntityPropertySingleValue<Property> propertyValue(std::move(extractedValue));
					properties[propertyIndex] = std::move(propertyValue);
				}
			};

			switch (propertyData.type)
			{
#define TSOM_ENTITYPROPERTYTYPE(V, T, IT) case EntityPropertyType:: T: Deserialize(EntityPropertyTag<EntityPropertyType:: T>{}); break;

#include <CommonLib/EntityPropertyList.hpp>
			}
		}

		return properties;
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

				if constexpr (IsArray)
				{
					auto elementArray = nlohmann::json::array();
					for (std::size_t i = 0; i < propertyValue.size(); ++i)
						elementArray.push_back(propertyValue[i]);

					propertiesObject[propertyName] = std::move(elementArray);
				}
				else
					propertiesObject[propertyName] = propertyValue.value;

			}, properties[propertyIndex]);
		}

		return propertiesObject;
	}
}
