// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/Entities/EntityClassLibrary.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <entt/entt.hpp>

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
				auto& instance = view.get<ClassInstanceComponent>(entity);

				auto refreshIt = m_refreshMap.find(instance.GetClass().get());
				if (refreshIt == m_refreshMap.end())
					continue; // class wasn't touched

				instance.UpdateClass(refreshIt->second);
			}
		}

		m_refreshMap.clear();
	}

	void EntityRegistry::RegisterClass(EntityClass entityClass)
	{
		std::string name = entityClass.GetName();
		m_classes[std::move(name)] = std::make_shared<EntityClass>(std::move(entityClass));
	}

	void EntityRegistry::RegisterClassLibrary(std::unique_ptr<EntityClassLibrary>&& library)
	{
		library->Register(*this);
		m_classLibraries.push_back(std::move(library));
	}
}
