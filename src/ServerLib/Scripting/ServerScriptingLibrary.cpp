// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerAtmosphere.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
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

		RegisterAtmosphere(state);
		RegisterEnvironment(state);
		RegisterPlayer(state);
		RegisterServer(state);
	}

	void ServerScriptingLibrary::RegisterAtmosphere(sol::state& state)
	{
		state.new_enum("GasType",
			"CarbonDioxyde", GasType::CarbonDioxyde,
			"Nitrogen", GasType::Nitrogen,
			"Oxygen", GasType::Oxygen
		);

		state.new_usertype<ServerAtmosphere>("Atmosphere",
			sol::no_constructor,
			"Exchange", LuaFunction([](ServerAtmosphere& serverAtmosphere, sol::stack_table exchangeGas)
			{
				Nz::EnumArray<GasType, Nz::Int32> gasExchange;
				gasExchange.fill(0);

				for (auto&& [key, value] : exchangeGas)
				{
					GasType gasType = key.as<GasType>();
					Nz::Int32 amount = value.as<Nz::Int32>();

					gasExchange[gasType] = amount;
				}

				return serverAtmosphere.Exchange(gasExchange);
			}),
			"GetGasAmount", LuaFunction(&ServerAtmosphere::GetGasAmount),
			"SetGasAmount", LuaFunction(&ServerAtmosphere::SetGasAmount)
		);
	}

	void ServerScriptingLibrary::RegisterEnvironment(sol::state& state)
	{
		state.new_enum("EnvironmentType",
			"Planet", ServerEnvironmentType::Planet,
			"Ship", ServerEnvironmentType::Ship
		);

		state.new_usertype<ServerEnvironment>("Environment",
			sol::no_constructor,

			"GetAllAtmospheres", LuaFunction([](ServerEnvironment* environment, sol::this_state L)
			{
				sol::state_view state(L);
				sol::table atmosphereTable = state.create_table();
				lua_Integer index = 1;
				environment->ForEachAtmosphere([&](ServerAtmosphere* atmosphere)
				{
					atmosphereTable[index++] = atmosphere;
				});

				return atmosphereTable;
			}),

			"GetAtmosphereAtPosition", LuaFunction(&ServerEnvironment::GetAtmosphereAtPosition),
			"GetPhysWorld", LuaFunction([](ServerEnvironment* environment)
			{
				return &environment->GetWorld().GetSystem<Nz::Physics3DSystem>();
			}),
			"GetType", LuaFunction(&ServerEnvironment::GetType),

			"IsRoot", LuaFunction(&ServerEnvironment::IsRoot)
		);

		state.new_usertype<ServerPlanetEnvironment>("PlanetEnvironment",
			sol::no_constructor,
			sol::base_classes, sol::bases<ServerEnvironment>()
		);

		state.new_usertype<ServerShipEnvironment>("ShipEnvironment",
			sol::no_constructor,
			sol::base_classes, sol::bases<ServerEnvironment>(),
			"GetExteriorShipEntity", LuaFunction([this](sol::this_state L, ServerShipEnvironment& shipEnvironment)
			{
				sol::state_view stateView(L);
				return m_entityScriptingLibrary.ToEntityTable(stateView, shipEnvironment.GetExteriorShipEntity());
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
			"GetEnvironment", LuaFunction([](ServerPlayer& player)
			{
				return player.GetControlledEntityEnvironment();
			}),
			"GetName", LuaFunction(&ServerPlayer::GetNickname),
			"GetPlayerIndex", LuaFunction(&ServerPlayer::GetPlayerIndex),
			"GetRootEnvironment", LuaFunction([](ServerPlayer& player)
			{
				return player.GetRootEnvironment();
			}),
			"GetUuid", LuaFunction([](ServerPlayer& player, sol::this_state L) -> sol::object
			{
				auto uuidOpt = player.GetUuid();
				if (!uuidOpt)
					return sol::nil;

				return sol::make_object(L, uuidOpt->ToString());
			})
		);
	}

	void ServerScriptingLibrary::RegisterServer(sol::state& state)
	{
		state.create_named_table("server",
			"Execute", LuaFunction([this](sol::this_state L, std::string_view command)
			{
				auto& fs = m_serverInstance.GetApplication().GetComponent<Nz::FilesystemAppComponent>();

				Nz::Result<sol::object, std::string> commandResult;
				std::string filepath = fmt::format("scripts/commands/{0}.lua", command);
				bool found = fs.GetFileContent(filepath, [&](const void* data, Nz::UInt64 size)
				{
					std::string_view scriptContent(static_cast<const char*>(data), Nz::SafeCast<std::size_t>(size));
					sol::state_view state(L);
					auto result = state.do_string(scriptContent, filepath, sol::load_mode::text);
					if (result.valid())
						commandResult = Nz::Ok(result.get<sol::object>());
					else
					{
						sol::error err = result;
						commandResult = Nz::Err(err.what());
					}
				});

				if (!found)
					commandResult = Nz::Err(fmt::format("file {} not found", filepath));

				if (!commandResult)
					throw std::runtime_error(fmt::format("Failed to execute command {0}: {1}", command, commandResult.GetError()));
			}),
			"FindPlayerByName", LuaFunction([this](std::string_view name)
			{
				return ServerPlayerHandle(m_serverInstance.FindPlayerByNickname(name));
			}),
			"FindPlayerByUuid", LuaFunction([this](std::string_view uuidStr)
			{
				Nz::Uuid uuid = Nz::Uuid::FromString(uuidStr);
				if (uuid.IsNull())
					throw std::runtime_error("invalid uuid");

				return ServerPlayerHandle(m_serverInstance.FindPlayerByUuid(uuid));
			}),
			"GetAllPlayers", LuaFunction([this]
			{
				std::vector<ServerPlayerHandle> players;
				m_serverInstance.ForEachPlayer([&](ServerPlayer& player)
				{
					players.push_back(player.CreateHandle());
				});

				return players;
			}),
			"ScheduleForNextTick", LuaFunction([this](sol::protected_function callback)
			{
				// It's possible for a ScriptingContext to schedule a callback just before getting destroyed
				m_serverInstance.ScheduleForNextTick([cb = std::move(callback), alive = m_aliveSignal]()
				{
					if (!*alive)
						return;

					auto result = cb();
					if (!result.valid())
					{
						sol::error err = result;
						fmt::print(fg(fmt::color::red), "Function scheduled for tick failed: {0}\n", err.what());
					}
				});
			})
		);
	}
}
