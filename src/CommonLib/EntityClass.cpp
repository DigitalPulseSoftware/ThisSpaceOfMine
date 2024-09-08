// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/EntityClassComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void EntityClass::InitializeEntity(entt::handle entity) const
	{
		entity.emplace<EntityClassComponent>().entityClass = this;

		if (m_callbacks.onInit)
			m_callbacks.onInit(entity);
	}
}
