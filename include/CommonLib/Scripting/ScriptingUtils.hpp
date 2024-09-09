// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_SCRIPTINGUTILS_HPP
#define TSOM_COMMONLIB_SCRIPTING_SCRIPTINGUTILS_HPP

#include <CommonLib/Export.hpp>
#include <entt/fwd.hpp>
#include <sol/sol.hpp>

namespace tsom
{
	TSOM_COMMONLIB_API entt::handle AssertScriptEntity(sol::table entityTable);
	TSOM_COMMONLIB_API entt::handle RetrieveScriptEntity(sol::table entityTable);

	template<typename... Args> [[noreturn]] void TriggerLuaError(lua_State* L, const char* format, Args&&... args);
	[[noreturn]] TSOM_COMMONLIB_API void TriggerLuaError(lua_State* L, const std::string& errMessage);
	[[noreturn]] TSOM_COMMONLIB_API void TriggerLuaArgError(lua_State* L, int argIndex, const char* errMessage);
	[[noreturn]] TSOM_COMMONLIB_API void TriggerLuaArgError(lua_State* L, int argIndex, const std::string& errMessage);

	template<typename F> auto LuaFunction(F funcPtr);
}

#include <CommonLib/Scripting/ScriptingUtils.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_SCRIPTINGUTILS_HPP
