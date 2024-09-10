// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/Entities/EntityClassLibrary.hpp>

namespace tsom
{
	EntityRegistry::EntityRegistry() = default;

	EntityRegistry::~EntityRegistry() = default;

	void EntityRegistry::RegisterClass(EntityClass entityClass)
	{
		std::string name = entityClass.GetName();
		m_classes.emplace(std::move(name), std::move(entityClass));
	}

	void EntityRegistry::RegisterClassLibrary(std::unique_ptr<EntityClassLibrary>&& library)
	{
		library->Register(*this);
		m_classLibraries.push_back(std::move(library));
	}
}
