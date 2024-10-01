// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <sol/state.hpp>

SOL_BASE_CLASSES(tsom::ServerPlanetEnvironment, tsom::ServerEnvironment);
SOL_BASE_CLASSES(tsom::ServerShipEnvironment, tsom::ServerEnvironment);
SOL_DERIVED_CLASSES(tsom::ServerEnvironment, tsom::ServerPlanetEnvironment, tsom::ServerShipEnvironment);

namespace tsom
{
	void ServerScriptingLibrary::Register(sol::state& state)
	{
		state["CLIENT"] = false;
		state["SERVER"] = true;

		RegisterEnvironment(state);
		RegisterPlayer(state);
	}

	void ServerScriptingLibrary::RegisterEnvironment(sol::state& state)
	{
		state.new_enum<ServerEnvironmentType>("EnvironmentType", {
			{ "Planet", ServerEnvironmentType::Planet },
			{ "Ship", ServerEnvironmentType::Ship },
		});

		state.new_usertype<ServerEnvironment>("Environment",
			sol::no_constructor,
			"GetType", LuaFunction(&ServerEnvironment::GetType)
		);

		state.new_usertype<ServerPlanetEnvironment>("PlanetEnvironment",
			sol::no_constructor,
			sol::base_classes, sol::bases<ServerEnvironment>()
		);

		state.new_usertype<ServerShipEnvironment>("ShipEnvironment",
			sol::no_constructor,
			sol::base_classes, sol::bases<ServerEnvironment>(),
			"GetOutsideShipEntity", LuaFunction([this](sol::this_state L, ServerShipEnvironment& shipEnvironment)
			{
				sol::state_view stateView(L);
				return m_entityScriptingLibrary.ToEntityTable(stateView, shipEnvironment.GetOutsideShipEntity());
			}),
			"GetShipEntity", LuaFunction([this](sol::this_state L, ServerShipEnvironment& shipEnvironment)
			{
				sol::state_view stateView(L);
				return m_entityScriptingLibrary.ToEntityTable(stateView, shipEnvironment.GetShipEntity());
			})
		);
	}

	void ServerScriptingLibrary::RegisterPlayer(sol::state& state)
	{
		state.new_usertype<ServerPlayerHandle>("Player",
			sol::no_constructor,
			"GetController", LuaFunction(&ServerPlayer::GetCharacterController),
			"GetName", LuaFunction(&ServerPlayer::GetNickname),
			"GetPlayerIndex", LuaFunction(&ServerPlayer::GetPlayerIndex),
			"GetUuid", LuaFunction(&ServerPlayer::GetUuid)
		);
	}
}
