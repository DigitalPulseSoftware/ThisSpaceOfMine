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
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <frozen/string.h>
#include <frozen/unordered_map.h>

SOL_BASE_CLASSES(Nz::BoxCollider3D, Nz::Collider3D);
SOL_DERIVED_CLASSES(Nz::Collider3D, Nz::BoxCollider3D);

namespace tsom
{
	namespace
	{
		struct EntityBuilder
		{
			sol::table classMetatable;
			std::vector<EntityClass::Property> properties;
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

							if (sol::optional<std::string_view> layerOpt = parameters["objectLayer"])
							{
								static constexpr auto s_layerMapping = frozen::make_unordered_map<frozen::string, Nz::PhysObjectLayer3D>({
									{ "dynamic", Constants::ObjectLayerDynamic },
									{ "dynamic_noplayer", Constants::ObjectLayerDynamicNoPlayer },
									{ "dynamic_trigger", Constants::ObjectLayerDynamicTrigger },
									{ "player", Constants::ObjectLayerPlayer },
									{ "static", Constants::ObjectLayerStatic },
									{ "static_noplayer", Constants::ObjectLayerStaticNoPlayer },
									{ "static_trigger", Constants::ObjectLayerStaticTrigger }
								});

								auto it = s_layerMapping.find(*layerOpt);
								if (it == s_layerMapping.end())
									throw std::runtime_error(fmt::format("invalid objectLayer {}", *layerOpt));

								commonSettings.objectLayer = it->second;
							}
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
			"On", LuaFunction([](EntityBuilder& entityBuilder, std::string_view eventName, sol::protected_function callback)
			{
				if (eventName != "init")
					throw std::runtime_error(fmt::format("unknown event {}", eventName));

				entityBuilder.classMetatable["_Init"] = std::move(callback);
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
		m_entityMetatable["AddComponent"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view componentType, sol::optional<sol::table> parameters)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			AddComponentFunc addComponent = RetrieveAddComponentHandler(componentType);
			if (!addComponent)
				throw std::runtime_error(fmt::format("invalid component {}", componentType));

			return addComponent(L, entity, parameters);
		});

		m_entityMetatable["GetComponent"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view componentType) -> sol::object
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			GetComponentFunc getComponent = RetrieveGetComponentHandler(componentType);
			if (!getComponent)
				throw std::runtime_error(fmt::format("invalid component {}", componentType));

			return getComponent(L, entity);
		});

		m_entityMetatable["GetProperty"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view propertyName)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& classComponent = entity.get<ClassInstanceComponent>();
			Nz::UInt32 propertyIndex = classComponent.entityClass->FindProperty(propertyName);
			if (propertyIndex == EntityClass::InvalidIndex)
				TriggerLuaArgError(L, 2, fmt::format("invalid property {}", propertyName));

			sol::state_view state(L);
			return TranslatePropertyToLua(state, classComponent.properties[propertyIndex]);
		});

		m_entityMetatable["SetProperty"] = LuaFunction([this](sol::this_state L, sol::table entityTable, std::string_view propertyName, sol::object value)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& classComponent = entity.get<ClassInstanceComponent>();
			Nz::UInt32 propertyIndex = classComponent.entityClass->FindProperty(propertyName);
			if (propertyIndex == EntityClass::InvalidIndex)
				TriggerLuaArgError(L, 2, fmt::format("invalid property {}", propertyName));

			const auto& property = classComponent.entityClass->GetProperty(propertyIndex);
			classComponent.properties[propertyIndex] = TranslatePropertyFromLua(value, property.type, property.isArray);
		});
	}

	void EntityScriptingLibrary::RegisterEntityRegistry(sol::state& state)
	{
		sol::table entityRegistry = state.create_named_table("EntityRegistry");
		entityRegistry["ClassBuilder"] = LuaFunction([this](sol::this_state L)
		{
			sol::state_view state(L);
			sol::table metatable = state.create_table();
			metatable[sol::meta_method::index] = m_entityMetatable;

			EntityClass::Callbacks callbacks;
			callbacks.onInit = [metatable, state](entt::handle entity) mutable
			{
				auto& entityScripted = entity.emplace<ScriptedEntityComponent>();
				entityScripted.classMetatable = metatable;
				entityScripted.entityTable = state.create_table();
				entityScripted.entityTable[sol::metatable_key] = entityScripted.classMetatable;
				entityScripted.entityTable["_Entity"] = entity;

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

			return EntityBuilder{
				.classMetatable = metatable,
				.callbacks = std::move(callbacks)
			};
		});

		entityRegistry["RegisterClass"] = LuaFunction([this](std::string name, EntityBuilder entityBuilder)
		{
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
