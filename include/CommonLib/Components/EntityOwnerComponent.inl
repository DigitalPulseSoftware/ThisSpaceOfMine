// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	EntityOwnerComponent::~EntityOwnerComponent()
	{
		for (entt::handle& handle : m_entities)
		{
			if (handle.valid())
				handle.destroy();
		}
	}

	inline void EntityOwnerComponent::Register(entt::handle entity)
	{
		m_entities.push_back(entity);
	}
}
