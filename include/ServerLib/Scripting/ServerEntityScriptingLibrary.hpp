// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SCRIPTING_SERVERENTITYSCRIPTINGLIBRARY_HPP
#define TSOM_SERVERLIB_SCRIPTING_SERVERENTITYSCRIPTINGLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Scripting/SharedEntityScriptingLibrary.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerEntityScriptingLibrary final : public SharedEntityScriptingLibrary
	{
		public:
			using SharedEntityScriptingLibrary::SharedEntityScriptingLibrary;

			void Register(sol::state& state) override;

		private:
			void FillEntityMetatable(sol::state& state, sol::table entityMetatable) override;

			void HandleInit(sol::table classMetatable, entt::handle entity) override;

			bool RegisterEvent(sol::table classMetatable, std::string_view eventName, sol::protected_function callback) override;

			void RegisterServerComponents(sol::state& state);

			AddComponentFunc RetrieveAddComponentHandler(std::string_view componentType) override;
			GetComponentFunc RetrieveGetComponentHandler(std::string_view componentType) override;
	};
}

#include <ServerLib/Scripting/ServerEntityScriptingLibrary.inl>

#endif // TSOM_SERVERLIB_SCRIPTING_SERVERENTITYSCRIPTINGLIBRARY_HPP
