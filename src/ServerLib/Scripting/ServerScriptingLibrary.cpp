// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <CommonLib/Planet.hpp>
#include <ServerLib/ServerAtmosphere.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Database/ServerDatabase.hpp>
#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <sol/state.hpp>
#include <spdlog/spdlog.h>

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
		RegisterServerDatabase(state);
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

			"CreateEntity", LuaFunction([&](ServerEnvironment* environment, std::string_view entityType, std::optional<sol::stack_table> parameters, sol::this_state L)
			{
				std::shared_ptr<const EntityClass> entityClass = m_serverInstance.GetEntityRegistry().FindClass(entityType);
				if (!entityClass)
					TriggerLuaArgError(L, 1, fmt::format("unknown entity class {}", entityType));

				Nz::Vector3f position = Nz::Vector3f::Zero();
				Nz::Quaternionf rotation = Nz::Quaternionf::Identity();

				if (parameters)
				{
					position = parameters->get_or("position", position);
					rotation = parameters->get_or("rotation", rotation);
				}

				entt::handle entity = environment->CreateEntity();

				entity.emplace<Nz::NodeComponent>(position, rotation);
				entity.emplace<NetworkedComponent>();
				entity.emplace<ClassInstanceComponent>(entityClass);

				entityClass->InitAndActivateEntity(entity);

				sol::state_view stateView(L);
				return m_entityScriptingLibrary.ToEntityTable(stateView, entity);
			}),

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
			sol::base_classes, sol::bases<ServerEnvironment>(),
			sol::meta_function::construct, sol::factories([this](std::optional<Nz::UInt32> databaseId, std::string generatorName, const Nz::Vector3ui& chunkCount, const std::string& type)
			{
				// TODO: Build properties from Lua table
				return std::make_unique<ServerPlanetEnvironment>(m_serverInstance, databaseId, std::move(generatorName), chunkCount, type, std::vector<EntityProperty>{});
			}),
			"GetDatabaseId", LuaFunction(&ServerPlanetEnvironment::GetDatabaseId),
			"GetChunkContainer", LuaFunction([](ServerPlanetEnvironment& env) -> ChunkContainer* { return &env.GetPlanet(); })
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
			"ExitPiloting", LuaFunction([](ServerPlayer& player)
			{
				return player.ExitPiloting();
			}),
			"GetController", LuaFunction(&ServerPlayer::GetCharacterController),
			"GetEntity", LuaFunction([this](ServerPlayer& player, sol::this_state L)
			{
				sol::state_view stateView(L);
				return m_entityScriptingLibrary.ToEntityTable(stateView, player.GetControlledEntityReference());
			}),
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
			}),
			"IsValid", &ServerPlayerHandle::IsValid,
			"PilotShip", LuaFunction([](ServerPlayer& player, sol::table shipEntity, sol::table shipExteriorEntity, const Nz::Quaternionf& referenceRotation)
			{
				entt::handle entity = AssertScriptEntity(shipEntity);
				entt::handle exteriorEntity = AssertScriptEntity(shipExteriorEntity);
				return player.PilotShip(entity, exteriorEntity, referenceRotation);
			})
		);
	}

	void ServerScriptingLibrary::RegisterServer(sol::state& state)
	{
		state.create_named_table("Server",
			"Execute", LuaFunction([this](sol::this_state L, std::string_view command, sol::optional<sol::table> params)
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
					{
						sol::protected_function func = result;
						result = func(params);
						if (result.valid())
							commandResult = Nz::Ok(result.get<sol::object>());
						else
						{
							sol::error err = result;
							commandResult = Nz::Err(err.what());
						}
					}
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

				return commandResult.GetValue();
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
			"LinkDatabaseEnvironments", LuaFunction([this](Nz::UInt32 sourceDatabaseId, Nz::UInt32 destinationDatabaseId, const Nz::Vector3f& position)
			{
				m_serverInstance.LinkDatabaseEnvironments(sourceDatabaseId, destinationDatabaseId, position);
			}),
			"RegisterDatabaseEnvironment", LuaFunction([this](Nz::UInt32 databaseId, std::unique_ptr<ServerPlanetEnvironment>& environment) //< Since we have to take the unique_ptr by reference we can't take the base type
			{
				m_serverInstance.RegisterDatabaseEnvironment(databaseId, std::move(environment));
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
						spdlog::error("Function scheduled for tick failed: {0}", err.what());
					}
				});
			})
		);
	}

	void ServerScriptingLibrary::RegisterServerDatabase(sol::state& state)
	{
		state.create_named_table("ServerDatabase",
			"CreatePlanet", LuaFunction([this](sol::stack_table table)
			{
				std::string generatorName = table["generatorName"];
				std::string type = table["type"];

				Database::Planet planet;
				planet.chunkCount = table["chunkCount"];
				planet.generatorName = generatorName;
				planet.type = type;

				/*
				TODO
				sol::table properties = table["properties"];
				for (const auto& [key, value] : properties.pairs())
				{
				}*/

				return m_serverInstance.GetThreadServerDatabase().CreatePlanet(planet);
			}),
			"StorePlanetLink", LuaFunction([this](sol::stack_table table)
			{
				Database::PlanetLink planetLink;
				planetLink.sourcePlanet = table["sourcePlanet"];
				planetLink.destinationPlanet = table["destinationPlanet"];
				planetLink.position = table["position"];

				return m_serverInstance.GetThreadServerDatabase().StorePlanetLink(planetLink);
			})
		);
	}
}
