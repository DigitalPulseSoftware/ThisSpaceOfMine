// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITYCLASS_HPP
#define TSOM_COMMONLIB_ENTITYCLASS_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <entt/fwd.hpp>
#include <tsl/hopscotch_map.h>
#include <functional>
#include <string>
#include <vector>

namespace tsom
{
	struct ClassInstanceComponent;

	class TSOM_COMMONLIB_API EntityClass
	{
		public:
			struct Callbacks;
			struct Property;

			EntityClass(std::string name, std::vector<Property> properties, Callbacks callbacks);
			EntityClass(const EntityClass&) = delete;
			EntityClass(EntityClass&&) noexcept = default;
			~EntityClass() = default;

			void ActivateEntity(entt::handle entity) const;

			inline Nz::UInt32 FindProperty(std::string_view propertyName) const;

			inline const std::string& GetName() const;
			inline const Property& GetProperty(Nz::UInt32 propertyIndex) const;
			inline Nz::UInt32 GetPropertyCount() const;

			EntityClass& operator=(const EntityClass&) = delete;
			EntityClass& operator=(EntityClass&&) noexcept = default;

			struct Callbacks
			{
				std::function<void(entt::handle)> onInit;
			};

			struct Property
			{
				std::string name;
				EntityPropertyType type;
				EntityProperty defaultValue;
				bool isArray;
				bool isNetworked;
			};

			static constexpr Nz::UInt32 InvalidIndex = Nz::MaxValue();

		private:
			std::string m_name;
			std::vector<Property> m_properties;
			tsl::hopscotch_map<std::string, std::size_t, std::hash<std::string_view>, std::equal_to<>> m_propertyIndices;
			Callbacks m_callbacks;
	};
}

#include <CommonLib/EntityClass.inl>

#endif // TSOM_COMMONLIB_ENTITYCLASS_HPP
