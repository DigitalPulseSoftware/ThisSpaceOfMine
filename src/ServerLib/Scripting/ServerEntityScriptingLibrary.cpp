// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/ScriptedEntityComponent.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/AtmosphereMonitor.hpp>
#include <ServerLib/Components/ServerEnvironmentSwitchComponent.hpp>
#include <ServerLib/Components/ServerInteractibleComponent.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <frozen/string.h>
#include <frozen/unordered_map.h>

namespace tsom
{
	namespace
	{
		constexpr auto s_serverComponents = frozen::make_unordered_map<frozen::string, SharedEntityScriptingLibrary::ComponentEntry>({
			{
				"atmosphere_monitor", SharedEntityScriptingLibrary::ComponentEntry::Default<AtmosphereMonitor>()
			}
		});
	}

	void ServerEntityScriptingLibrary::Register(sol::state& state)
	{
		SharedEntityScriptingLibrary::Register(state);

		RegisterServerComponents(state);
	}

	void ServerEntityScriptingLibrary::FillEntityMetatable(sol::state& state, sol::table entityMetatable)
	{
		SharedEntityScriptingLibrary::FillEntityMetatable(state, entityMetatable);

		entityMetatable["AllowEnvironmentSwitch"] = LuaFunction([](sol::table entityTable)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			entity.emplace<ServerEnvironmentSwitchComponent>();
		});

		entityMetatable["CallClientRPC"] = LuaFunction([](sol::table entityTable, std::string rpcName, std::optional<ServerPlayerHandle> targetPlayer)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& instance = entity.get<ClassInstanceComponent>();
			Nz::UInt32 rpcIndex = instance.FindClientRpcIndex(rpcName);
			if (rpcIndex == EntityClass::InvalidIndex)
				throw std::runtime_error(fmt::format("client rpc {} doesn't exist", rpcName));

			instance.TriggerClientRpc(rpcIndex, targetPlayer.value_or(ServerPlayerHandle{}));
		});

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

		sol::optional<sol::protected_function> envSwitchCallback = classMetatable["_EnvSwitch"];
		if (envSwitchCallback)
		{
			auto& entityEnvSwitch = entity.get<ServerEnvironmentSwitchComponent>();
			entityEnvSwitch.handleEnvironmentSwitch = [cb = std::move(*envSwitchCallback)](entt::handle previousEntity, entt::handle newEntity, const EnvironmentTransform& /*relativeTransform*/)
			{
				auto& previousEntityScripted = previousEntity.get<ScriptedEntityComponent>();
				auto& newEntityScripted = newEntity.get<ScriptedEntityComponent>();

				auto res = cb(newEntityScripted.entityTable, previousEntityScripted.entityTable);
				if (!res.valid())
				{
					sol::error err = res;
					fmt::print(fg(fmt::color::red), "entity environment switch event failed: {}\n", err.what());
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
		else if (eventName == "env_switch")
		{
			classMetatable["_EnvSwitch"] = std::move(callback);
			return true;
		}

		return false;
	}

	void ServerEntityScriptingLibrary::RegisterServerComponents(sol::state& state)
	{
		state.new_usertype<AtmosphereMonitor>("AtmosphereMonitor",
			sol::no_constructor,
			"Atmosphere", sol::readonly_property(&AtmosphereMonitor::atmosphere));
	}

	auto ServerEntityScriptingLibrary::RetrieveAddComponentHandler(std::string_view componentType) -> AddComponentFunc
	{
		if (AddComponentFunc addComponentHandler = SharedEntityScriptingLibrary::RetrieveAddComponentHandler(componentType))
			return addComponentHandler;

		auto it = s_serverComponents.find(componentType);
		if (it == s_serverComponents.end())
			return nullptr;

		return it->second.addComponent;
	}

	auto ServerEntityScriptingLibrary::RetrieveGetComponentHandler(std::string_view componentType) -> GetComponentFunc
	{
		if (GetComponentFunc getComponentHandler = SharedEntityScriptingLibrary::RetrieveGetComponentHandler(componentType))
			return getComponentHandler;

		auto it = s_serverComponents.find(componentType);
		if (it == s_serverComponents.end())
			return nullptr;

		return it->second.getComponent;
	}
}
