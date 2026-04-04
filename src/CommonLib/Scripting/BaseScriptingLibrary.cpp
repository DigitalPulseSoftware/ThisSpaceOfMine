// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/BaseScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Core/Time.hpp>
#include <fmt/format.h>
#include <sol/sol.hpp>

namespace tsom
{
	void BaseScriptingLibrary::Register(sol::state& state)
	{
		state["CreateMetatable"] = [](sol::this_state L, const char* metaname)
		{
			if (luaL_newmetatable(L, metaname) == 0)
			{
				lua_pop(L, 1);
				TriggerLuaArgError(L, 1, fmt::format("Metatable %s already exists", metaname));
			}

			return sol::stack_table(L);
		};

		state["GetMetatable"]= [](sol::this_state L, const char* metaname)
		{
			luaL_getmetatable(L, metaname);
			return sol::stack_table(L);
		};

		RegisterTime(state);
	}

	void BaseScriptingLibrary::RegisterTime(sol::state& state)
	{
		state.new_usertype<Nz::Time>("Time",
			sol::no_constructor,
			"AsMicroseconds", &Nz::Time::AsMicroseconds,
			"AsMilliseconds", &Nz::Time::AsMilliseconds,
			"AsNanoseconds", &Nz::Time::AsNanoseconds,
			"AsSeconds", &Nz::Time::AsSeconds<float>,
			"Milliseconds", &Nz::Time::Milliseconds,
			"Microseconds", &Nz::Time::Microseconds,
			"Nanosecond", &Nz::Time::Nanosecond,
			"Second", &Nz::Time::Second,
			"Seconds", &Nz::Time::Seconds<float>,
			"Zero", &Nz::Time::Zero,
			sol::meta_function::equal_to, [](Nz::Time t1, Nz::Time t2) { return t1 == t2; },
			sol::meta_function::less_than, [](Nz::Time t1, Nz::Time t2) { return t1 < t2; },
			sol::meta_function::less_than_or_equal_to, [](Nz::Time t1, Nz::Time t2) { return t1 <= t2; },
			sol::meta_function::addition, [](Nz::Time t1, Nz::Time t2) { return t1 + t2; },
			sol::meta_function::subtraction, [](Nz::Time t1, Nz::Time t2) { return t1 - t2; },
			sol::meta_function::unary_minus, [](Nz::Time t) { return -t; }
		);
	}
}
