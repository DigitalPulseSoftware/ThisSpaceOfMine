// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/SharedEntityScriptingLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <CommonLib/Components/ScriptedEntityComponent.hpp>
#include <CommonLib/Components/TickComponent.hpp>
#include <CommonLib/Scripting/ScriptingProperties.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <Nazara/Physics3D/Components/PhysCharacter3DComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <frozen/string.h>
#include <frozen/unordered_map.h>
#include <spdlog/spdlog.h>

SOL_BASE_CLASSES(Nz::BoxCollider3D, Nz::Collider3D);
SOL_BASE_CLASSES(Nz::NodeComponent, Nz::Node);
SOL_BASE_CLASSES(Nz::PhysCharacter3DComponent, Nz::PhysCharacter3D);
SOL_BASE_CLASSES(Nz::RigidBody3DComponent, Nz::RigidBody3D);
SOL_DERIVED_CLASSES(Nz::Collider3D, Nz::BoxCollider3D);
SOL_DERIVED_CLASSES(Nz::Node, Nz::NodeComponent);
SOL_DERIVED_CLASSES(Nz::PhysCharacter3D, Nz::PhysCharacter3DComponent);
SOL_DERIVED_CLASSES(Nz::RigidBody3D, Nz::RigidBody3DComponent);

namespace tsom
{
	namespace
	{
		struct EntityBuilder
		{
			sol::table classMetatable;
			std::vector<EntityClass::RemoteProcedureCall> clientRpcs;
			std::vector<EntityClass::RemoteProcedureCall> serverRpcs;
			std::vector<EntityClass::Property> properties;
			std::vector<sol::protected_function> propertyUpdateCallbacks;
			Nz::ParameterList metadata;
			EntityClass::Callbacks callbacks;
		};

		constexpr auto s_components = frozen::make_unordered_map<frozen::string, SharedEntityScriptingLibrary::ComponentEntry>({
			{
				"node", SharedEntityScriptingLibrary::ComponentEntry::Default<Nz::NodeComponent>()
			},
			{
				"physicscharacter3d", SharedEntityScriptingLibrary::ComponentEntry{
					.addComponent = [](sol::this_state L, entt::handle entity, sol::optional<sol::table> parametersOpt) -> sol::object
					{
						throw std::runtime_error("physicscharacter3d cannot be added from Lua");
					},
					.getComponent = SharedEntityScriptingLibrary::ComponentEntry::DefaultGet<Nz::PhysCharacter3DComponent>()
				}
			},
			{
				"rigidbody3d", SharedEntityScriptingLibrary::ComponentEntry{
					.addComponent = [](sol::this_state L, entt::handle entity, sol::optional<sol::table> parametersOpt)
					{
						if (!parametersOpt)
							throw std::runtime_error("missing parameters");

						sol::table& parameters = *parametersOpt;
						std::string kind = parameters["kind"];

						auto HandleCommonParameters = [](Nz::RigidBody3DComponent::CommonSettings& commonSettings, sol::table& parameters)
						{
							commonSettings.collider = parameters.get_or<std::shared_ptr<Nz::Collider3D>>("collider", commonSettings.collider);
							commonSettings.initiallySleeping = parameters.get_or("initiallySleeping", commonSettings.initiallySleeping);
							commonSettings.isSimulationEnabled = parameters.get_or("isSimulationEnabled", commonSettings.isSimulationEnabled);
							commonSettings.isTrigger = parameters.get_or("isTrigger", commonSettings.isTrigger);
							commonSettings.objectLayer = parameters.get_or("objectLayer", commonSettings.objectLayer);
						};

						if (kind == "dynamic")
						{
							Nz::RigidBody3DComponent::DynamicSettings dynamicSettings;
							HandleCommonParameters(dynamicSettings, parameters);
							dynamicSettings.allowSleeping = parameters.get_or("allowSleeping", dynamicSettings.allowSleeping);
							dynamicSettings.angularDamping = parameters.get_or("angularDamping", dynamicSettings.angularDamping);
							dynamicSettings.friction = parameters.get_or("friction", dynamicSettings.friction);
							dynamicSettings.gravityFactor = parameters.get_or("gravityFactor", dynamicSettings.gravityFactor);
							dynamicSettings.linearDamping = parameters.get_or("linearDamping", dynamicSettings.linearDamping);
							dynamicSettings.mass = parameters.get_or("mass", dynamicSettings.mass);
							dynamicSettings.maxAngularVelocity = parameters.get_or("maxAngularVelocity", dynamicSettings.maxAngularVelocity);
							dynamicSettings.maxLinearVelocity = parameters.get_or("maxLinearVelocity", dynamicSettings.maxLinearVelocity);
							dynamicSettings.restitution = parameters.get_or("restitution", dynamicSettings.restitution);

							auto& rigidBody = entity.emplace<Nz::RigidBody3DComponent>(dynamicSettings);
							return sol::make_object(L, &rigidBody);
						}
						else if (kind == "static")
						{
							Nz::RigidBody3DComponent::StaticSettings staticSettings;
							HandleCommonParameters(staticSettings, parameters);

							auto& rigidBody = entity.emplace<Nz::RigidBody3DComponent>(staticSettings);
							return sol::make_object(L, &rigidBody);
						}
						else
							throw std::runtime_error("invalid kind " + kind);
					},
					.getComponent = SharedEntityScriptingLibrary::ComponentEntry::DefaultGet<Nz::RigidBody3DComponent>()
				}
			},
			{
				"distribution", SharedEntityScriptingLibrary::ComponentEntry{
					.addComponent = [](sol::this_state L, entt::handle entity, sol::optional<sol::table> parameters)
					{
						if (!parameters)
							throw std::runtime_error("missing parameters");

						sol::table tableInputs = (*parameters)["inputs"];

						std::size_t inputCount = tableInputs.size();
						std::vector<DistributionType> inputs(inputCount);
						for (std::size_t i = 0; i < inputCount; ++i)
							inputs[i] = tableInputs[i + 1];

						sol::table tableOutputs = (*parameters)["outputs"];

						std::size_t outputCount = tableOutputs.size();
						std::vector<DistributionType> outputs(outputCount);
						for (std::size_t i = 0; i < outputCount; ++i)
							outputs[i] = tableOutputs[i + 1];

						return sol::make_object(L, &entity.emplace<DistributionComponent>(inputs, outputs));
					},
					.getComponent = SharedEntityScriptingLibrary::ComponentEntry::DefaultGet<DistributionComponent>()
				}
			}
		});
	}

	SharedEntityScriptingLibrary::~SharedEntityScriptingLibrary() = default;

	void SharedEntityScriptingLibrary::Register(sol::state& state)
	{
		RegisterConstants(state);

		RegisterComponents(state);
		RegisterEntityBuilder(state);
		RegisterEntityMetatable(state);
		RegisterEntityRegistry(state);
		RegisterPhysics(state);
	}

	sol::object SharedEntityScriptingLibrary::ToEntityTable(sol::state_view& state, entt::handle entity)
	{
		if (!entity)
			return sol::lua_nil;

		if (ScriptedEntityComponent* scriptedComponent = entity.try_get<ScriptedEntityComponent>())
		{
			if (scriptedComponent->entityTable.lua_state() == state)
				return scriptedComponent->entityTable;
		}

		//TODO: Expose the correct class metatable in this state if possible
		sol::table entityTable = state.create_table();
		entityTable["_Entity"] = EntityReference(entity);
		entityTable[sol::metatable_key] = m_entityMetatable;

		return entityTable;
	}

	void SharedEntityScriptingLibrary::FillConstants(sol::state& state, sol::table constants)
	{
		// Game
		constants["PlayerOxygenConsumption"] = Constants::PlayerOxygenConsumption;
		constants["DistributionTickRate"] = Constants::DistributionTickRate;

		// Internal
		constants["TickDuration"] = Constants::TickDuration;

		// Physics
		constants["BroadphaseStatic"] = Constants::BroadphaseStatic;
		constants["BroadphaseDynamic"] = Constants::BroadphaseDynamic;
		constants["ObjectLayerDynamic"] = Constants::ObjectLayerDynamic;
		constants["ObjectLayerDynamicNoCollision"] = Constants::ObjectLayerDynamicNoCollision;
		constants["ObjectLayerDynamicNoPlayer"] = Constants::ObjectLayerDynamicNoPlayer;
		constants["ObjectLayerDynamicTrigger"] = Constants::ObjectLayerDynamicTrigger;
		constants["ObjectLayerPlayer"] = Constants::ObjectLayerPlayer;
		constants["ObjectLayerPlayerOnlyTrigger"] = Constants::ObjectLayerPlayerOnlyTrigger;
		constants["ObjectLayerStatic"] = Constants::ObjectLayerStatic;
		constants["ObjectLayerStaticNoPlayer"] = Constants::ObjectLayerStaticNoPlayer;
		constants["ObjectLayerStaticTrigger"] = Constants::ObjectLayerStaticTrigger;
	}

	void SharedEntityScriptingLibrary::FillEntityMetatable(sol::state& state, sol::table entityMetatable)
	{
		entityMetatable["AddComponent"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view componentType, sol::optional<sol::table> parameters)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			AddComponentFunc addComponent = RetrieveAddComponentHandler(componentType);
			if (!addComponent)
				throw std::runtime_error(fmt::format("invalid component {}", componentType));

			return addComponent(L, entity, parameters);
		});

		entityMetatable["GetComponent"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view componentType) -> sol::object
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			GetComponentFunc getComponent = RetrieveGetComponentHandler(componentType);
			if (!getComponent)
				throw std::runtime_error(fmt::format("invalid component {}", componentType));

			return getComponent(L, entity);
		});

		entityMetatable["GetProperty"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view propertyName)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& classComponent = entity.get<ClassInstanceComponent>();
			Nz::UInt32 propertyIndex = classComponent.FindPropertyIndex(propertyName);
			if (propertyIndex == EntityClass::InvalidIndex)
				TriggerLuaArgError(L, 2, fmt::format("invalid property {}", propertyName));

			sol::state_view state(L);
			return TranslatePropertyToLua(state, classComponent.GetProperty(propertyIndex));
		});

		entityMetatable["SetTickInterval"] = LuaFunction([this](sol::this_state L, sol::table entityTable, Nz::UInt32 milliseconds)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			TickComponent* entityTick = entity.try_get<TickComponent>();
			if (!entityTick)
				TriggerLuaError(L, "entity has no tick callback");

			entityTick->tickRate = Nz::Time::Milliseconds(milliseconds);
		});

		entityMetatable["UpdateProperty"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view propertyName, sol::object value)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& classInstance = entity.get<ClassInstanceComponent>();
			Nz::UInt32 propertyIndex = classInstance.FindPropertyIndex(propertyName);
			if (propertyIndex == EntityClass::InvalidIndex)
				TriggerLuaArgError(L, 2, fmt::format("invalid property {}", propertyName));

			const auto& property = classInstance.GetClass()->GetProperty(propertyIndex);
			classInstance.UpdateProperty(propertyIndex, TranslatePropertyFromLua(value, property.type, property.isArray));
		});
	}

	void SharedEntityScriptingLibrary::PostInit(sol::table classMetatable, entt::handle entity)
	{
	}

	bool SharedEntityScriptingLibrary::RegisterEvent(sol::table classMetatable, std::string_view eventName, sol::protected_function callback)
	{
		return false;
	}

	auto SharedEntityScriptingLibrary::RetrieveAddComponentHandler(std::string_view componentType) -> AddComponentFunc
	{
		auto it = s_components.find(componentType);
		if (it == s_components.end())
			return nullptr;

		return it->second.addComponent;
	}

	auto SharedEntityScriptingLibrary::RetrieveGetComponentHandler(std::string_view componentType) -> GetComponentFunc
	{
		auto it = s_components.find(componentType);
		if (it == s_components.end())
			return nullptr;

		return it->second.getComponent;
	}

	void SharedEntityScriptingLibrary::RegisterConstants(sol::state& state)
	{
		sol::table constantMetatable = state.create_table_with(
			sol::meta_function::index, [](sol::this_state L) { TriggerLuaError(L, "invalid constant"); },
			sol::meta_function::new_index, [](sol::this_state L) { TriggerLuaError(L, "Constants is read-only"); }
		);

		sol::table constants = state.create_named_table("Constants");
		FillConstants(state, constants);

		constants[sol::metatable_key] = constantMetatable;
	}

	void SharedEntityScriptingLibrary::RegisterComponents(sol::state& state)
	{
		state.new_usertype<Nz::NodeComponent>("NodeComponent",
			sol::no_constructor,
			"GetPosition", LuaFunction(&Nz::Node::GetPosition),
			"GetRotation", LuaFunction(&Nz::Node::GetRotation),
			"GetForward", LuaFunction(&Nz::Node::GetForward),
			"GetRight", LuaFunction(&Nz::Node::GetRight),
			"GetUp", LuaFunction(&Nz::Node::GetUp),
			"Scale", sol::overload(
				LuaFunction([](Nz::NodeComponent& nodeComponent, float scale)
				{
					return nodeComponent.Scale(scale);
				}),
				LuaFunction([](Nz::NodeComponent& nodeComponent, const Nz::Vector3f& scale)
				{
					return nodeComponent.Scale(scale);
				})
			),
			"SetPosition", LuaFunction([](Nz::NodeComponent& nodeComponent, const Nz::Vector3f& position)
			{
				return nodeComponent.SetPosition(position);
			}),
			"SetRotation", LuaFunction([](Nz::NodeComponent& nodeComponent, const Nz::Quaternionf& rotation)
			{
				return nodeComponent.SetRotation(rotation);
			}),
			"SetScale", sol::overload(
				LuaFunction([](Nz::NodeComponent& nodeComponent, float scale)
				{
					return nodeComponent.SetScale(scale);
				}),
				LuaFunction([](Nz::NodeComponent& nodeComponent, const Nz::Vector3f& position)
				{
					return nodeComponent.SetPosition(position);
				})
			),
			"SetTransform", LuaFunction([](Nz::NodeComponent& nodeComponent, const Nz::Vector3f& position, const Nz::Quaternionf& rotation, const Nz::Vector3f& scale)
			{
				return nodeComponent.SetTransform(position, rotation, scale);
			}),
			"ToGlobalDirection", LuaFunction(&Nz::NodeComponent::ToGlobalDirection),
			"ToGlobalPosition", LuaFunction(&Nz::NodeComponent::ToGlobalPosition),
			"ToGlobalRotation", LuaFunction(&Nz::NodeComponent::ToGlobalRotation),
			"ToGlobalScale", LuaFunction(&Nz::NodeComponent::ToGlobalScale),
			"ToLocalDirection", LuaFunction(&Nz::NodeComponent::ToLocalDirection),
			"ToLocalPosition", LuaFunction(&Nz::NodeComponent::ToLocalPosition),
			"ToLocalRotation", LuaFunction(&Nz::NodeComponent::ToLocalRotation),
			"ToLocalScale", LuaFunction(&Nz::NodeComponent::ToLocalScale)
		);

		state.new_usertype<Nz::PhysCharacter3DComponent>("PhysCharacter3DComponent",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::PhysCharacter3D>(),
			"GetAngularVelocity", LuaFunction(&Nz::PhysCharacter3DComponent::GetAngularVelocity),
			"GetCollider", LuaFunction(&Nz::PhysCharacter3DComponent::GetCollider),
			"GetLinearVelocity", LuaFunction(&Nz::PhysCharacter3DComponent::GetLinearVelocity),
			"GetPosition", LuaFunction(&Nz::PhysCharacter3DComponent::GetPosition),
			"GetObjectLayer", LuaFunction(&Nz::PhysCharacter3DComponent::GetObjectLayer),
			"GetRotation", LuaFunction(&Nz::PhysCharacter3DComponent::GetRotation),
			"SetAngularVelocity", LuaFunction(&Nz::PhysCharacter3DComponent::SetAngularVelocity),
			"SetLinearVelocity", LuaFunction(&Nz::PhysCharacter3DComponent::SetLinearVelocity),
			"SetObjectLayer", LuaFunction(&Nz::PhysCharacter3DComponent::SetObjectLayer),
			"SetRotation", LuaFunction(&Nz::PhysCharacter3DComponent::SetRotation),
			"TeleportTo", LuaFunction(&Nz::PhysCharacter3DComponent::TeleportTo)
		);

		state.new_usertype<Nz::RigidBody3DComponent>("Rigidbody3DComponent",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::RigidBody3D>(),
			"GetAABB", LuaFunction(&Nz::RigidBody3DComponent::GetAABB),
			"GetAngularDamping", LuaFunction(&Nz::RigidBody3DComponent::GetAngularDamping),
			"GetAngularVelocity", LuaFunction(&Nz::RigidBody3DComponent::GetAngularVelocity),
			"GetCollider", LuaFunction(&Nz::RigidBody3DComponent::GetCollider),
			"GetLinearDamping", LuaFunction(&Nz::RigidBody3DComponent::GetLinearDamping),
			"GetLinearVelocity", LuaFunction(&Nz::RigidBody3DComponent::GetLinearVelocity),
			"GetMass", LuaFunction(&Nz::RigidBody3DComponent::GetMass),
			"GetObjectLayer", LuaFunction(&Nz::RigidBody3DComponent::GetObjectLayer),
			"GetPosition", LuaFunction(&Nz::RigidBody3DComponent::GetPosition),
			"GetRotation", LuaFunction(&Nz::RigidBody3DComponent::GetRotation),
			"SetAngularDamping", LuaFunction(&Nz::RigidBody3DComponent::SetAngularDamping),
			"SetAngularVelocity", LuaFunction(&Nz::RigidBody3DComponent::SetAngularVelocity),
			"SetCollider", LuaFunction(&Nz::RigidBody3DComponent::SetCollider),
			"SetLinearDamping", LuaFunction(&Nz::RigidBody3DComponent::SetLinearDamping),
			"SetLinearVelocity", LuaFunction(&Nz::RigidBody3DComponent::SetLinearVelocity),
			"SetMass", LuaFunction(&Nz::RigidBody3DComponent::SetMass),
			"SetObjectLayer", LuaFunction(&Nz::RigidBody3DComponent::SetObjectLayer),
			"SetPosition", LuaFunction(&Nz::RigidBody3DComponent::SetPosition),
			"SetRotation", LuaFunction(&Nz::RigidBody3DComponent::SetRotation),
			"TeleportTo", LuaFunction(&Nz::RigidBody3DComponent::TeleportTo)
		);

		state.new_enum("DistributionType",
			"Electrical", DistributionType::Electrical,
			"Gas", DistributionType::Gas
		);

		state.new_usertype<ElectricalQuantity>("ElectricalQuantity",
			sol::call_constructor, [](std::uint32_t energy) { return ElectricalQuantity{ energy }; },
			"Minimize", &ElectricalQuantity::Minimize,
			"energy", &ElectricalQuantity::energy
		);

		state.new_usertype<GasQuantity>("GasQuantity",
			sol::call_constructor, sol::constructors<GasQuantity()>(),
			"Increment", &GasQuantity::Increment,
			"Minimize", &GasQuantity::Minimize,
			sol::meta_function::index, &GasQuantity::Get,
			sol::meta_function::new_index, &GasQuantity::Set
		);
	}

	void SharedEntityScriptingLibrary::RegisterEntityBuilder(sol::state& state)
	{
		state.new_usertype<EntityBuilder>("EntityBuilder",
			sol::no_constructor,
			"AddClientRPC", LuaFunction([](EntityBuilder& entityBuilder, std::string rpcName)
			{
				entityBuilder.clientRpcs.push_back({
					.name = std::move(rpcName)
				});
			}),
			"AddProperty", LuaFunction([](EntityBuilder& entityBuilder, std::string propertyName, sol::table propertyData)
			{
				std::string_view type = propertyData.get<std::string_view>("type");
				bool isArray = propertyData.get_or("isArray", false);
				bool isNetworked = propertyData.get_or("isNetworked", false);

				EntityPropertyType propertyType = ParseEntityPropertyType(type);
				EntityProperty entityProperty = TranslatePropertyFromLua(propertyData["default"], propertyType, isArray);

				entityBuilder.properties.push_back({
					.name = std::move(propertyName),
					.type = propertyType,
					.defaultValue = std::move(entityProperty),
					.isArray = isArray,
					.isNetworked = isNetworked
				});
			}),
			"Set", sol::overload(
				LuaFunction([this](EntityBuilder& entityBuilder, std::string parameter, bool value)
				{
					entityBuilder.metadata.SetParameter(std::move(parameter), value);
				}),
				LuaFunction([this](EntityBuilder& entityBuilder, std::string parameter, long long value)
				{
					entityBuilder.metadata.SetParameter(std::move(parameter), value);
				}),
				LuaFunction([this](EntityBuilder& entityBuilder, std::string parameter, double value)
				{
					entityBuilder.metadata.SetParameter(std::move(parameter), value);
				}),
				LuaFunction([this](EntityBuilder& entityBuilder, std::string parameter, std::string value)
				{
					entityBuilder.metadata.SetParameter(std::move(parameter), std::move(value));
				}),
				LuaFunction([this](EntityBuilder& entityBuilder, const std::string& parameter, const Nz::Vector3f& value)
				{
					entityBuilder.metadata.SetParameter(parameter + ".x", value.x);
					entityBuilder.metadata.SetParameter(parameter + ".y", value.y);
					entityBuilder.metadata.SetParameter(parameter + ".z", value.z);
				})
			),
			"On", LuaFunction([this](sol::this_state L, EntityBuilder& entityBuilder, std::string_view eventName, sol::protected_function callback)
			{
				if (eventName == "init")
					entityBuilder.classMetatable["_Init"] = std::move(callback);
				else if (eventName == "activate")
					entityBuilder.classMetatable["_Activate"] = std::move(callback);
				else if (eventName == "tick")
					entityBuilder.classMetatable["_Tick"] = std::move(callback);
				else
				{
					if (!RegisterEvent(entityBuilder.classMetatable, eventName, std::move(callback)))
						TriggerLuaError(L, fmt::format("unknown event {}", eventName));
				}
			}),
			"OnClientRPC", LuaFunction([this](sol::this_state L, EntityBuilder& entityBuilder, std::string eventName, sol::protected_function callback)
			{
				auto rpcIt = std::find_if(entityBuilder.clientRpcs.begin(), entityBuilder.clientRpcs.end(), [&](const EntityClass::RemoteProcedureCall& rpc) { return rpc.name == eventName; });
				if (rpcIt == entityBuilder.clientRpcs.end())
					TriggerLuaError(L, fmt::format("unknown client rpc {}", eventName));

				rpcIt->onCalled = [cb = std::move(callback), en = std::move(eventName)](entt::handle entity)
				{
					auto& entityScripted = entity.get<ScriptedEntityComponent>();

					auto res = cb(entityScripted.entityTable);
					if (!res.valid())
					{
						sol::error err = res;
						spdlog::error("entity client rpc {} failed: {}", en, err.what());
					}
				};
			}),
			"OnPropertyUpdate", LuaFunction([this](sol::this_state L, EntityBuilder& entityBuilder, std::string_view propertyName, sol::protected_function callback)
			{
				auto propertyIt = std::find_if(entityBuilder.properties.begin(), entityBuilder.properties.end(), [&](const EntityClass::Property& property) { return property.name == propertyName; });
				if (propertyIt == entityBuilder.properties.end())
					TriggerLuaArgError(L, 2, fmt::format("unknown property {}", propertyName));

				std::size_t propertyIndex = std::distance(entityBuilder.properties.begin(), propertyIt);

				if (propertyIndex >= entityBuilder.propertyUpdateCallbacks.size())
					entityBuilder.propertyUpdateCallbacks.resize(propertyIndex + 1);

				entityBuilder.propertyUpdateCallbacks[propertyIndex] = std::move(callback);
			}),
			sol::meta_method::index, LuaFunction([](EntityBuilder& entityBuilder, std::string_view key)
			{
				return entityBuilder.classMetatable.get<sol::object>(key);
			}),
			sol::meta_method::new_index, LuaFunction([](EntityBuilder& entityBuilder, std::string_view key, sol::object value)
			{
				return entityBuilder.classMetatable.set(key, std::move(value));
			})
		);
	}

	void SharedEntityScriptingLibrary::RegisterEntityMetatable(sol::state& state)
	{
		m_entityMetatable = state.create_table();
		m_entityMetatable[sol::meta_method::index] = m_entityMetatable;
		m_entityMetatable[sol::meta_method::equal_to] = [](sol::stack_table t1, sol::stack_table t2)
		{
			auto t1Entity = t1.get<sol::optional<EntityReference>>("_Entity");
			auto t2Entity = t2.get<sol::optional<EntityReference>>("_Entity");
			if (!t1Entity || !t2Entity)
				return false;

			return *t1Entity == *t2Entity;
		};

		FillEntityMetatable(state, m_entityMetatable);
	}

	void SharedEntityScriptingLibrary::RegisterEntityRegistry(sol::state& state)
	{
		sol::table entityRegistry = state.create_named_table("EntityRegistry");
		entityRegistry["ClassBuilder"] = LuaFunction([this](sol::this_state L)
		{
			sol::state_view state(L);
			sol::table metatable = state.create_table();
			metatable[sol::meta_method::index] = metatable;
			metatable[sol::metatable_key] = m_entityMetatable;

			return EntityBuilder{
				.classMetatable = metatable
			};
		});

		entityRegistry["RegisterClass"] = LuaFunction([this](sol::this_state L, std::string name, EntityBuilder entityBuilder)
		{
			sol::state_view state(L);

			std::shared_ptr sharedCallbacks = std::make_shared<std::vector<sol::protected_function>>(std::move(entityBuilder.propertyUpdateCallbacks));

			if (sol::optional<sol::protected_function> activateCallback = entityBuilder.classMetatable["_Activate"])
			{
				entityBuilder.callbacks.onActivate = [this, callback = std::move(activateCallback)](entt::handle entity) mutable
				{
					auto& entityScripted = entity.get<ScriptedEntityComponent>();

					auto res = (*callback)(entityScripted.entityTable);
					if (!res.valid())
					{
						sol::error err = res;
						spdlog::error("entity activate event failed: {}", err.what());
					}
				};
			}

			sol::optional<sol::protected_function> tickCallback = entityBuilder.classMetatable["_Tick"];

			entityBuilder.callbacks.onInit = [this, state, metatable = std::move(entityBuilder.classMetatable), sharedCallbacks, tickCallback](entt::handle entity) mutable
			{
				auto& entityInstance = entity.get<ClassInstanceComponent>();
				entityInstance.OnPropertyUpdate.Connect([entity, sharedCallbacks, state](ClassInstanceComponent* classInstance, Nz::UInt32 propertyIndex, const EntityProperty& newValue) mutable
				{
					auto& callbacks = (*sharedCallbacks);
					if (propertyIndex >= callbacks.size() || !callbacks[propertyIndex])
						return;

					auto& entityScripted = entity.get<ScriptedEntityComponent>();

					auto res = callbacks[propertyIndex](entityScripted.entityTable, TranslatePropertyToLua(state, newValue));
					if (!res.valid())
					{
						const auto& propertyData = classInstance->GetClass()->GetProperty(propertyIndex);

						sol::error err = res;
						spdlog::error("entity {} property callback failed: {}", propertyData.name, err.what());
					}
				});

				auto& entityScripted = entity.emplace<ScriptedEntityComponent>();
				entityScripted.classMetatable = metatable;
				entityScripted.entityTable = state.create_table();
				entityScripted.entityTable[sol::metatable_key] = entityScripted.classMetatable;
				entityScripted.entityTable["_Entity"] = EntityReference(entity);

				if (tickCallback)
				{
					auto& entityTick = entity.emplace<TickComponent>();
					entityTick.onTick = [onTick = *tickCallback](entt::handle entity)
					{
						auto& entityScripted = entity.get<ScriptedEntityComponent>();
						auto res = onTick(entityScripted.entityTable);
						if (!res.valid())
						{
							sol::error err = res;
							spdlog::error("entity tick callback failed: {}", err.what());
						}
					};
				}

				sol::optional<sol::protected_function> initCallback = entityScripted.classMetatable["_Init"];
				if (initCallback)
				{
					auto res = (*initCallback)(entityScripted.entityTable);
					if (!res.valid())
					{
						sol::error err = res;
						spdlog::error("entity init event failed: {}", err.what());
					}
				}

				PostInit(entityScripted.classMetatable, entity);
			};

			// TODO: In case of script reloading, we must update the classMetatable for existing entities as well

			m_entityRegistry.RegisterClass(EntityClass{ std::move(name), std::move(entityBuilder.properties), std::move(entityBuilder.callbacks), std::move(entityBuilder.clientRpcs), std::move(entityBuilder.serverRpcs), std::move(entityBuilder.metadata) });
		});
	}

	void SharedEntityScriptingLibrary::RegisterPhysics(sol::state& state)
	{
		state.new_usertype<Nz::Collider3D>("Collider3D",
			sol::no_constructor,
			"GetBoundingBox", &Nz::Collider3D::GetBoundingBox,
			"GetCenterOfMass", &Nz::Collider3D::GetCenterOfMass
		);

		state.new_usertype<Nz::BoxCollider3D>("BoxCollider3D",
			sol::base_classes, sol::bases<Nz::Collider3D>(),
			sol::meta_function::construct, sol::factories(LuaFunction([](const Nz::Vector3f& lengths) { return std::make_shared<Nz::BoxCollider3D>(lengths); }))
		);

		state.new_usertype<Nz::TranslatedRotatedCollider3D>("TranslatedRotatedCollider3D",
			sol::base_classes, sol::bases<Nz::Collider3D>(),
			sol::meta_function::construct, sol::factories(
				LuaFunction([](std::shared_ptr<Nz::Collider3D> collider, const Nz::Vector3f& translation) { return std::make_shared<Nz::TranslatedRotatedCollider3D>(std::move(collider), translation); }),
				LuaFunction([](std::shared_ptr<Nz::Collider3D> collider, const Nz::Quaternionf& rotation) { return std::make_shared<Nz::TranslatedRotatedCollider3D>(std::move(collider), rotation); }),
				LuaFunction([](std::shared_ptr<Nz::Collider3D> collider, const Nz::Vector3f& translation, const Nz::Quaternionf& rotation) { return std::make_shared<Nz::TranslatedRotatedCollider3D>(std::move(collider), translation, rotation); })
			)
		);
	}
}
