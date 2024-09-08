// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityRegistry.hpp>

namespace tsom
{
	void EntityRegistry::RegisterClass(EntityClass entityClass)
	{
		std::string name = entityClass.GetName();
		m_classes.emplace(std::move(name), std::move(entityClass));
	}
}
