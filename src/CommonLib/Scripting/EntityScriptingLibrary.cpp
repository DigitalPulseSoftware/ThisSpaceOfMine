// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/EntityScriptingLibrary.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/ScriptedEntityComponent.hpp>
#include <CommonLib/Scripting/ScriptingProperties.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <frozen/string.h>
#include <frozen/unordered_map.h>

SOL_BASE_CLASSES(Nz::BoxCollider3D, Nz::Collider3D);
SOL_BASE_CLASSES(Nz::RigidBody3DComponent, Nz::RigidBody3D);
SOL_DERIVED_CLASSES(Nz::Collider3D, Nz::BoxCollider3D);
SOL_DERIVED_CLASSES(Nz::RigidBody3D, Nz::RigidBody3DComponent);

namespace tsom
{
	namespace
	{
		struct EntityBuilder
		{
			sol::table classMetatable;
			std::vector<EntityClass::Property> properties;
			std::vector<sol::protected_function> propertyUpdateCallbacks;
			EntityClass::Callbacks callbacks;
		};

		constexpr auto s_components = frozen::make_unordered_map<frozen::string, EntityScriptingLibrary::ComponentEntry>({
			{
				"node", EntityScriptingLibrary::ComponentEntry::Default<Nz::NodeComponent>()
			},
			{
				"rigidbody3d", EntityScriptingLibrary::ComponentEntry{
					.addComponent = [](sol::this_state L, entt::handle entity, sol::optional<sol::table> parametersOpt)
					{
						if (!parametersOpt)
							throw std::runtime_error("missing parameters");

						sol::table& parameters = *parametersOpt;
						std::string kind = parameters["kind"];

						auto HandleCommonParameters = [](Nz::RigidBody3DComponent::CommonSettings& commonSettings, sol::table& parameters)
						{
							commonSettings.geom = parameters.get<std::shared_ptr<Nz::Collider3D>>("geom");
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
					.getComponent = EntityScriptingLibrary::ComponentEntry::DefaultGet<Nz::RigidBody3DComponent>()
				}
			}
		});
	}

	EntityScriptingLibrary::~EntityScriptingLibrary() = default;

	void EntityScriptingLibrary::Register(sol::state& state)
	{
		RegisterConstants(state);

		RegisterComponents(state);
		RegisterEntityBuilder(state);
		RegisterEntityMetatable(state);
		RegisterEntityRegistry(state);
		RegisterPhysics(state);
	}

	void EntityScriptingLibrary::FillConstants(sol::state& state, sol::table constants)
	{
		// Internal
		constants["TickDuration"] = Constants::TickDuration;

		// Physics
		constants["BroadphaseStatic"] = Constants::BroadphaseStatic;
		constants["BroadphaseDynamic"] = Constants::BroadphaseDynamic;
		constants["ObjectLayerDynamic"] = Constants::ObjectLayerDynamic;
		constants["ObjectLayerDynamicNoPlayer"] = Constants::ObjectLayerDynamicNoPlayer;
		constants["ObjectLayerDynamicTrigger"] = Constants::ObjectLayerDynamicTrigger;
		constants["ObjectLayerPlayer"] = Constants::ObjectLayerPlayer;
		constants["ObjectLayerStatic"] = Constants::ObjectLayerStatic;
		constants["ObjectLayerStaticNoPlayer"] = Constants::ObjectLayerStaticNoPlayer;
		constants["ObjectLayerStaticTrigger"] = Constants::ObjectLayerStaticTrigger;
	}

	void EntityScriptingLibrary::FillEntityMetatable(sol::state& state, sol::table entityMetatable)
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

	void EntityScriptingLibrary::HandleInit(sol::table classMetatable, entt::handle entity)
	{
	}

	bool EntityScriptingLibrary::RegisterEvent(sol::table classMetatable, std::string_view eventName, sol::protected_function callback)
	{
		return false;
	}

	auto EntityScriptingLibrary::RetrieveAddComponentHandler(std::string_view componentType) -> AddComponentFunc
	{
		auto it = s_components.find(componentType);
		if (it == s_components.end())
			return nullptr;

		return it->second.addComponent;
	}

	auto EntityScriptingLibrary::RetrieveGetComponentHandler(std::string_view componentType) -> GetComponentFunc
	{
		auto it = s_components.find(componentType);
		if (it == s_components.end())
			return nullptr;

		return it->second.getComponent;
	}

	void EntityScriptingLibrary::RegisterConstants(sol::state& state)
	{
		sol::table constantMetatable = state.create_table_with(
			sol::meta_function::index, [](sol::this_state L) { TriggerLuaError(L, "invalid constant"); },
			sol::meta_function::new_index, [](sol::this_state L) { TriggerLuaError(L, "Constants is read-only"); }
		);

		sol::table constants = state.create_named_table("Constants");
		FillConstants(state, constants);

		constants[sol::metatable_key] = constantMetatable;
	}

	void EntityScriptingLibrary::RegisterComponents(sol::state& state)
	{
		state.new_usertype<Nz::NodeComponent>("NodeComponent",
			sol::no_constructor,
			"Scale", LuaFunction([](Nz::NodeComponent& nodeComponent, const Nz::Vector3f& scale)
			{
				return nodeComponent.Scale(scale);
			})
		);

		state.new_usertype<Nz::RigidBody3DComponent>("Rigidbody3DComponent",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::RigidBody3D>(),
			"GetAABB", LuaFunction(&Nz::RigidBody3DComponent::GetAABB),
			"GetAngularDamping", LuaFunction(&Nz::RigidBody3DComponent::GetAngularDamping),
			"GetAngularVelocity", LuaFunction(&Nz::RigidBody3DComponent::GetAngularVelocity),
			"GetGeom", LuaFunction(&Nz::RigidBody3DComponent::GetGeom),
			"GetLinearDamping", LuaFunction(&Nz::RigidBody3DComponent::GetLinearDamping),
			"GetLinearVelocity", LuaFunction(&Nz::RigidBody3DComponent::GetLinearVelocity),
			"GetMass", LuaFunction(&Nz::RigidBody3DComponent::GetMass),
			"GetPosition", LuaFunction(&Nz::RigidBody3DComponent::GetPosition),
			"GetRotation", LuaFunction(&Nz::RigidBody3DComponent::GetRotation),
			"SetAngularDamping", LuaFunction(&Nz::RigidBody3DComponent::SetAngularDamping),
			"SetAngularVelocity", LuaFunction(&Nz::RigidBody3DComponent::SetAngularVelocity),
			"SetGeom", LuaFunction(&Nz::RigidBody3DComponent::SetGeom),
			"SetLinearDamping", LuaFunction(&Nz::RigidBody3DComponent::SetLinearDamping),
			"SetLinearVelocity", LuaFunction(&Nz::RigidBody3DComponent::SetLinearVelocity),
			"SetMass", LuaFunction(&Nz::RigidBody3DComponent::SetMass),
			"SetObjectLayer", LuaFunction(&Nz::RigidBody3DComponent::SetObjectLayer),
			"SetPosition", LuaFunction(&Nz::RigidBody3DComponent::SetPosition),
			"SetRotation", LuaFunction(&Nz::RigidBody3DComponent::SetRotation),
			"TeleportTo", LuaFunction(&Nz::RigidBody3DComponent::TeleportTo)
		);
	}

	void EntityScriptingLibrary::RegisterEntityBuilder(sol::state& state)
	{
		state.new_usertype<EntityBuilder>("EntityBuilder",
			sol::no_constructor,
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
			"On", LuaFunction([this](sol::this_state L, EntityBuilder& entityBuilder, std::string_view eventName, sol::protected_function callback)
			{
				if (eventName == "init")
					entityBuilder.classMetatable["_Init"] = std::move(callback);
				else
				{
					if (!RegisterEvent(entityBuilder.classMetatable, eventName, std::move(callback)))
						TriggerLuaError(L, fmt::format("unknown event {}", eventName));
				}
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

	void EntityScriptingLibrary::RegisterEntityMetatable(sol::state& state)
	{
		m_entityMetatable = state.create_table();
		m_entityMetatable[sol::meta_method::index] = m_entityMetatable;

		FillEntityMetatable(state, m_entityMetatable);
	}

	void EntityScriptingLibrary::RegisterEntityRegistry(sol::state& state)
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
			entityBuilder.callbacks.onInit = [this, state, metatable = std::move(entityBuilder.classMetatable), sharedCallbacks](entt::handle entity) mutable
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
						fmt::print(fg(fmt::color::red), "entity {} property callback failed: {}\n", propertyData.name, err.what());
					}
				});

				auto& entityScripted = entity.emplace<ScriptedEntityComponent>();
				entityScripted.classMetatable = metatable;
				entityScripted.entityTable = state.create_table();
				entityScripted.entityTable[sol::metatable_key] = entityScripted.classMetatable;
				entityScripted.entityTable["_Entity"] = entity;

				HandleInit(entityScripted.classMetatable, entity);

				sol::optional<sol::protected_function> initCallback = entityScripted.classMetatable["_Init"];
				if (initCallback)
				{
					auto res = (*initCallback)(entityScripted.entityTable);
					if (!res.valid())
					{
						sol::error err = res;
						fmt::print(fg(fmt::color::red), "entity init event failed: {}\n", err.what());
					}
				}
			};

			m_entityRegistry.RegisterClass(EntityClass{std::move(name), std::move(entityBuilder.properties), std::move(entityBuilder.callbacks)});
		});
	}

	void EntityScriptingLibrary::RegisterPhysics(sol::state& state)
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
	}
}
