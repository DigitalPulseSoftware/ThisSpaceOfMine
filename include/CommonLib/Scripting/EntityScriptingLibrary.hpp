// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_ENTITYSCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_ENTITYSCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>
#include <entt/entt.hpp>
#include <sol/state.hpp>

namespace tsom
{
	class EntityRegistry;

	class TSOM_COMMONLIB_API EntityScriptingLibrary : public ScriptingLibrary
	{
		public:
			inline EntityScriptingLibrary(EntityRegistry& entityRegistry);
			EntityScriptingLibrary(const EntityScriptingLibrary&) = delete;
			EntityScriptingLibrary(EntityScriptingLibrary&&) = delete;
			virtual ~EntityScriptingLibrary();

			virtual void Register(sol::state& state) override;

			EntityScriptingLibrary& operator=(const EntityScriptingLibrary&) = delete;
			EntityScriptingLibrary& operator=(EntityScriptingLibrary&&) = delete;

			using AddComponentFunc = sol::object(*)(sol::this_state L, entt::handle entity, sol::optional<sol::table> parameters);
			using GetComponentFunc = sol::object(*)(sol::this_state L, entt::handle entity);

			struct ComponentEntry
			{
				EntityScriptingLibrary::AddComponentFunc addComponent;
				EntityScriptingLibrary::GetComponentFunc getComponent;

				template<typename T> static constexpr AddComponentFunc DefaultAdd();
				template<typename T> static constexpr GetComponentFunc DefaultGet();
				template<typename T> static constexpr ComponentEntry Default();
			};

		protected:
			virtual void FillConstants(sol::state& state, sol::table constants);

			virtual AddComponentFunc RetrieveAddComponentHandler(std::string_view componentType);
			virtual GetComponentFunc RetrieveGetComponentHandler(std::string_view componentType);

		private:
			void RegisterComponents(sol::state& state);
			void RegisterConstants(sol::state& state);
			void RegisterEntityBuilder(sol::state& state);
			void RegisterEntityMetatable(sol::state& state);
			void RegisterEntityRegistry(sol::state& state);
			void RegisterPhysics(sol::state& state);

			EntityRegistry& m_entityRegistry;
			sol::table m_entityMetatable;
	};
}

#include <CommonLib/Scripting/EntityScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_ENTITYSCRIPTINGLIBRARY_HPP
