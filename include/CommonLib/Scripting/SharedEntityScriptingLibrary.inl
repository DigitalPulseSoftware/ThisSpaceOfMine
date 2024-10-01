// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline SharedEntityScriptingLibrary::SharedEntityScriptingLibrary(EntityRegistry& entityRegistry) :
	m_entityRegistry(entityRegistry)
	{
	}

	template<typename T>
	constexpr auto SharedEntityScriptingLibrary::ComponentEntry::DefaultAdd() -> AddComponentFunc
	{
		return [](sol::this_state L, entt::handle entity, sol::optional<sol::table> /*parameters*/)
		{
			return sol::make_object(L, &entity.emplace<T>());
		};
	}

	template<typename T>
	constexpr auto SharedEntityScriptingLibrary::ComponentEntry::DefaultGet() -> GetComponentFunc
	{
		return [](sol::this_state L, entt::handle entity)
		{
			return sol::make_object(L, &entity.get<T>());
		};
	}

	template<typename T>
	constexpr auto SharedEntityScriptingLibrary::ComponentEntry::Default() -> ComponentEntry
	{
		return ComponentEntry{
			.addComponent = DefaultAdd<T>(),
			.getComponent = DefaultGet<T>()
		};
	}
}
