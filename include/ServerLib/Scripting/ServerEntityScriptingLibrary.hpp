// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SCRIPTING_SERVERENTITYSCRIPTINGLIBRARY_HPP
#define TSOM_SERVERLIB_SCRIPTING_SERVERENTITYSCRIPTINGLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Scripting/EntityScriptingLibrary.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerEntityScriptingLibrary final : public EntityScriptingLibrary
	{
		public:
			using EntityScriptingLibrary::EntityScriptingLibrary;

		private:
			void FillEntityMetatable(sol::state& state, sol::table entityMetatable) override;

			void HandleInit(sol::table classMetatable, entt::handle entity) override;

			bool RegisterEvent(sol::table classMetatable, std::string_view eventName, sol::protected_function callback) override;
	};
}

#include <ServerLib/Scripting/ServerEntityScriptingLibrary.inl>

#endif // TSOM_SERVERLIB_SCRIPTING_SERVERENTITYSCRIPTINGLIBRARY_HPP
