// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <sstream>

namespace tsom
{
	namespace
	{
		int PushAsString(lua_State* L, int idx, bool extended);
		int PushTableAsString(lua_State* L);

		int PushAsString(lua_State* L, int idx, bool extended)
		{
			if (!luaL_callmeta(L, idx, "__tostring"))
			{
				int t = lua_type(L, idx);

				switch (t)
				{
					case LUA_TNIL:
						lua_pushliteral(L, "nil");
						break;

					case LUA_TSTRING:
						lua_pushvalue(L, idx);
						break;

					case LUA_TNUMBER:
						if (lua_isinteger(L, idx))
							lua_pushfstring(L, "%I", (LUAI_UACINT)lua_tointeger(L, idx));
						else
							lua_pushfstring(L, "%f", (LUAI_UACNUMBER)lua_tonumber(L, idx));
						break;

					case LUA_TBOOLEAN:
						if (lua_toboolean(L, idx))
							lua_pushliteral(L, "true");
						else
							lua_pushliteral(L, "false");
						break;

					case LUA_TTABLE:
					{
						if (extended)
						{
							lua_pushvalue(L, idx);
							lua_pushliteral(L, "");
							lua_createtable(L, 0, 0);
							PushTableAsString(L);
							break;
						}

						[[fallthrough]];
					}

					default:
					{
						int tt = luaL_getmetafield(L, idx, "__name");
						const char* name = (tt == LUA_TSTRING) ? lua_tostring(L, idx) : lua_typename(L, t);
						lua_pushfstring(L, "%s: %p", name, lua_topointer(L, idx));
						if (tt != LUA_TNIL)
							lua_replace(L, -2);
						break;
					}
				}
			}
			else
			{
				if (!lua_isstring(L, -1))
					luaL_error(L, "'__tostring' must return a string");
			}

			return 1;
		}

		int PushTableAsString(lua_State* L)
		{
			int tableIndex = lua_absindex(L, -3);
			int indentIndex = lua_absindex(L, -2);
			int visitedIndex = lua_absindex(L, -1);

			luaL_checktype(L, tableIndex, LUA_TTABLE); //< table
			luaL_checktype(L, indentIndex, LUA_TSTRING); //< indentation
			luaL_checktype(L, visitedIndex, LUA_TTABLE); //< visited

			// Add indentation
			lua_pushvalue(L, indentIndex);
			lua_pushstring(L, "  ");
			lua_concat(L, 2);
			int newIndentIndex = lua_absindex(L, -1); //< new value on top of the stack is the new indentation

			luaL_Buffer b;
			luaL_buffinit(L, &b);

			lua_pushvalue(L, indentIndex);
			luaL_addvalue(&b);

			lua_pushstring(L, "{\n");
			luaL_addvalue(&b);

			lua_pushnil(L);
			while (lua_next(L, tableIndex))
			{
				bool recursive = false;
				if (lua_istable(L, -1))
				{
					lua_pushvalue(L, -1);
					lua_gettable(L, visitedIndex);
					if (lua_isnil(L, -1))
					{
						lua_pop(L, 1);

						lua_pushvalue(L, -1);
						lua_pushboolean(L, true);
						lua_settable(L, visitedIndex);

						recursive = true;
					}
				}

				// Indent
				lua_pushvalue(L, newIndentIndex);
				luaL_addvalue(&b);

				// Key
				PushAsString(L, -2, false);
				luaL_addvalue(&b);

				// = 
				lua_pushstring(L, (recursive) ? " = \n" : " = ");
				luaL_addvalue(&b);

				// Value
				if (recursive)
				{
					lua_pushvalue(L, -1);
					lua_pushvalue(L, newIndentIndex);
					lua_pushvalue(L, visitedIndex);
					PushTableAsString(L);
				}
				else
					PushAsString(L, -1, false);

				luaL_addvalue(&b);

				lua_pushstring(L, "\n");
				luaL_addvalue(&b);

				lua_pop(L, 1);
			}

			lua_pop(L, 2); // pop new indentation and visited table

			luaL_addvalue(&b); //< add (and pop) new indentation to the buffer

			lua_pushstring(L, "}");
			luaL_addvalue(&b);

			lua_pop(L, 1); //< pop table

			luaL_pushresult(&b);
			return 1;
		}
	}

	ScriptingContext::ScriptingContext(Nz::ApplicationBase& app) :
	m_app(app)
	{
		m_state.open_libraries();

		m_state["tostring"] = [](lua_State* L) -> int
		{
			luaL_checkany(L, 1);
			bool extended = luaL_opt(L, lua_toboolean, 2, false);
			PushAsString(L, 1, extended);
			return 1;
		};

		auto PrintExt = [this](sol::this_state L, sol::variadic_args args, bool extended)
		{
			bool first = true;

			std::ostringstream oss;
			for (auto v : args)
			{
				PushAsString(L, v.stack_index(), extended);

				std::size_t length;
				const char* str = lua_tolstring(L, -1, &length);
				oss << std::string(str, length);
				if (!first)
					oss << "\t";

				first = false;
			}

			if (m_printCallback)
				m_printCallback(std::move(oss).str());
			else
				fmt::print("[Lua] {}\n", oss.str());
		};

		m_state["pprint"] = [=](sol::this_state L, sol::variadic_args args)
		{
			PrintExt(L, args, true);
		};

		m_state["print"] = [=](sol::this_state L, sol::variadic_args args)
		{
			PrintExt(L, args, false);
		};
	}

	ScriptingContext::~ScriptingContext() = default;

	Nz::Result<sol::object, std::string> ScriptingContext::Execute(std::string_view str, const std::string& origin)
	{
		sol::protected_function_result result = m_state.do_string(str, origin, sol::load_mode::text);
		if (!result.valid())
		{
			sol::error err = result;
			return Nz::Err(err.what());
		}

		return Nz::Ok(result.get<sol::object>());
	}

	void ScriptingContext::LoadDirectory(std::string_view directoryPath)
	{
		auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();
		fs.IterateOnDirectory(directoryPath, [&](std::string_view path, const Nz::VirtualDirectory::Entry& dirEntry)
		{
			if (!std::holds_alternative<Nz::VirtualDirectory::FileEntry>(dirEntry))
				return; //< TODO: Allow recursive?

			LoadFile(fmt::format("{}/{}", directoryPath, path));
		});
	}

	Nz::Result<sol::object, std::string> ScriptingContext::LoadFile(const std::string& filePath)
	{
		auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();

		Nz::Result<sol::object, std::string> result;
		bool found = fs.GetFileContent(filePath, [&](const void* data, Nz::UInt64 size)
		{
			std::string_view scriptContent(static_cast<const char*>(data), Nz::SafeCast<std::size_t>(size));
			result = Execute(scriptContent, filePath);
		});

		if (!found)
			result = Nz::Err(fmt::format("file {} not found", filePath));

		if (!result)
			fmt::print(fg(fmt::color::red), "failed to load {}: {}\n", filePath, result.GetError());

		return result;
	}

	auto ScriptingContext::OverridePrintCallback(PrintCallback&& printCallback) -> PrintCallback
	{
		return std::exchange(m_printCallback, std::move(printCallback));
	}

	ScriptingLibrary& ScriptingContext::RegisterLibrary(std::unique_ptr<ScriptingLibrary>&& library)
	{
		library->Register(m_state);
		m_libraries.push_back(std::move(library));

		return *m_libraries.back();
	}
}
