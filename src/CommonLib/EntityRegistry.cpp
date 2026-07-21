// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Entities/EntityClassLibrary.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>

namespace tsom
{
	EntityRegistry::EntityRegistry() :
	m_isRefreshing(false)
	{
	}

	EntityRegistry::~EntityRegistry() = default;

	void EntityRegistry::Refresh(std::span<entt::registry*> registries, Nz::FunctionRef<void()> refreshCallback)
	{
		m_isRefreshing = true;
		refreshCallback();
		m_isRefreshing = false;

		for (entt::registry* reg : registries)
		{
			auto view = reg->view<ClassInstanceComponent>();
			for (auto [entity, classInstance] : view.each())
			{
				auto refreshIt = m_refreshMap.find(classInstance.GetClass().get());
				if (refreshIt == m_refreshMap.end())
					continue; // class wasn't touched

				classInstance.UpdateClass(refreshIt->second);
			}
		}

		m_refreshMap.clear();
	}

	std::shared_ptr<const EntityClass> EntityRegistry::RegisterClass(EntityClass entityClass)
	{
		std::string name = entityClass.GetName();
		std::shared_ptr<EntityClass> entityClassPtr = std::make_shared<EntityClass>(std::move(entityClass));

		if (auto it = m_classes.find(name); it != m_classes.end())
		{
			if (!m_isRefreshing)
				throw std::runtime_error(fmt::format("Class {} is already registered", name));

			m_refreshMap.emplace(it->second.get(), entityClassPtr);
			it.value() = entityClassPtr;
		}
		else
			m_classes[std::move(name)] = entityClassPtr;

		return entityClassPtr;
	}

	void EntityRegistry::RegisterClassLibrary(std::unique_ptr<EntityClassLibrary>&& library)
	{
		library->Register(*this);
		m_classLibraries.push_back(std::move(library));
	}
}
