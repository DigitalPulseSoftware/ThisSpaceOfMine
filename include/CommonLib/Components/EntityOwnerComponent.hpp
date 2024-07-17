// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_ENTITYOWNERCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_ENTITYOWNERCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace tsom
{
	class EntityOwnerComponent
	{
		public:
			EntityOwnerComponent() = default;
			EntityOwnerComponent(const EntityOwnerComponent&) = delete;
			EntityOwnerComponent(EntityOwnerComponent&&) = default;
			inline ~EntityOwnerComponent();

			inline void Register(entt::handle entity);

			EntityOwnerComponent& operator=(const EntityOwnerComponent&) = delete;
			EntityOwnerComponent& operator=(EntityOwnerComponent&&) = default;

		private:
			std::vector<entt::handle> m_entities;
	};
}

#include <CommonLib/Components/EntityOwnerComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_ENTITYOWNERCOMPONENT_HPP
