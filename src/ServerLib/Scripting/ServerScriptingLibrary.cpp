// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <sol/state.hpp>

namespace tsom
{
	void ServerScriptingLibrary::Register(sol::state& state)
	{
		state["CLIENT"] = false;
		state["SERVER"] = true;

		RegisterPlayer(state);
	}

	void ServerScriptingLibrary::RegisterPlayer(sol::state& state)
	{
		state.new_usertype<ServerPlayerHandle>("Player",
			sol::no_constructor,
			"GetName", LuaFunction(&ServerPlayer::GetNickname),
			"GetPlayerIndex", LuaFunction(&ServerPlayer::GetPlayerIndex),
			"GetUuid", LuaFunction(&ServerPlayer::GetUuid)
		);
	}
}
