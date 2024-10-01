// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <CommonLib/Components/ScriptedEntityComponent.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/ServerInteractibleComponent.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	void ServerEntityScriptingLibrary::FillEntityMetatable(sol::state& state, sol::table entityMetatable)
	{
		SharedEntityScriptingLibrary::FillEntityMetatable(state, entityMetatable);

		entityMetatable["GetEnvironment"] = LuaFunction([](sol::this_state L, sol::table entityTable) -> sol::object
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			entt::registry* reg = entity.registry();
			if (ServerPlanetEnvironment** planetEnv = reg->ctx().find<ServerPlanetEnvironment*>())
				return sol::make_object(L, *planetEnv);
			else if (ServerShipEnvironment** shipEnv = reg->ctx().find<ServerShipEnvironment*>())
				return sol::make_object(L, *shipEnv);
			else
				throw std::runtime_error("no environment found");
		});

		entityMetatable["SetInteractible"] = LuaFunction([](sol::this_state L, sol::table entityTable, bool isInteractible)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			ServerInteractibleComponent* interactibleComponent = entity.try_get<ServerInteractibleComponent>();
			if (isInteractible)
			{
				if (!interactibleComponent || !interactibleComponent->onInteraction)
					TriggerLuaError(L, "entity has no interact callback");
			}

			interactibleComponent->isEnabled = isInteractible;
		});
	}

	void ServerEntityScriptingLibrary::HandleInit(sol::table classMetatable, entt::handle entity)
	{
		sol::optional<sol::protected_function> interactCallback = classMetatable["_Interact"];
		if (interactCallback)
		{
			auto& entityInteractible = entity.emplace<ServerInteractibleComponent>();
			entityInteractible.isEnabled = false;
			entityInteractible.onInteraction = [cb = std::move(*interactCallback)](entt::handle entity, ServerPlayer* triggeringPlayer)
			{
				auto& entityScripted = entity.get<ScriptedEntityComponent>();

				auto res = cb(entityScripted.entityTable, (triggeringPlayer) ? triggeringPlayer->CreateHandle() : Nz::ObjectHandle<ServerPlayer>{});
				if (!res.valid())
				{
					sol::error err = res;
					fmt::print(fg(fmt::color::red), "entity interact event failed: {}\n", err.what());
				}
			};
		}
	}

	bool ServerEntityScriptingLibrary::RegisterEvent(sol::table classMetatable, std::string_view eventName, sol::protected_function callback)
	{
		if (eventName == "interact")
		{
			classMetatable["_Interact"] = std::move(callback);
			return true;
		}

		return false;
	}
}
