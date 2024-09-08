// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITYCLASS_HPP
#define TSOM_COMMONLIB_ENTITYCLASS_HPP

#include <CommonLib/Export.hpp>
#include <entt/fwd.hpp>
#include <functional>
#include <string>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API EntityClass
	{
		public:
			struct Callbacks;
			struct Property;

			inline EntityClass(std::string name, std::vector<Property> properties, Callbacks callbacks);
			EntityClass(const EntityClass&) = delete;
			EntityClass(EntityClass&&) noexcept = default;
			~EntityClass() = default;

			inline const std::string& GetName() const;

			void InitializeEntity(entt::handle entity) const;

			EntityClass& operator=(const EntityClass&) = delete;
			EntityClass& operator=(EntityClass&&) noexcept = default;

			struct Callbacks
			{
				std::function<void(entt::handle)> onInit;
			};

			struct Property
			{
				std::string name;
			};

		private:
			Callbacks m_callbacks;
			std::string m_name;
			std::vector<Property> m_properties;
	};
}

#include <CommonLib/EntityClass.inl>

#endif // TSOM_COMMONLIB_ENTITYCLASS_HPP
