// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientEntityScriptingLibrary.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/ClientInteractibleComponent.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Graphics/PointLight.hpp>
#include <Nazara/Graphics/SpotLight.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Graphics/Components/LightComponent.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <frozen/string.h>
#include <frozen/unordered_map.h>

namespace Nz
{
	class Model;
}

SOL_BASE_CLASSES(Nz::Model, Nz::InstancedRenderable);
SOL_DERIVED_CLASSES(Nz::InstancedRenderable, Nz::Model);

namespace tsom
{
	namespace
	{
		constexpr auto s_clientComponents = frozen::make_unordered_map<frozen::string, SharedEntityScriptingLibrary::ComponentEntry>({
			{
				"graphics", SharedEntityScriptingLibrary::ComponentEntry::Default<Nz::GraphicsComponent>()
			},
			{
				"light", SharedEntityScriptingLibrary::ComponentEntry::Default<Nz::LightComponent>()
			}
		});
	}

	void ClientEntityScriptingLibrary::Register(sol::state& state)
	{
		SharedEntityScriptingLibrary::Register(state);

		RegisterClientComponents(state);
	}

	void ClientEntityScriptingLibrary::FillConstants(sol::state& state, sol::table constants)
	{
		SharedEntityScriptingLibrary::FillConstants(state, constants);

		constants["RenderMask2D"] = Constants::RenderMask2D;
		constants["RenderMaskUI"] = Constants::RenderMaskUI;
		constants["RenderMask3D"] = Constants::RenderMask3D;
		constants["RenderMaskLocalPlayer"] = Constants::RenderMaskLocalPlayer;
		constants["RenderMaskOtherPlayer"] = Constants::RenderMaskOtherPlayer;
	}

	void ClientEntityScriptingLibrary::FillEntityMetatable(sol::state& state, sol::table entityMetatable)
	{
		SharedEntityScriptingLibrary::FillEntityMetatable(state, entityMetatable);

		entityMetatable["SetInteractible"] = LuaFunction([](sol::table entityTable, bool isInteractible)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			if (isInteractible)
			{
				auto& interactible = entity.get_or_emplace<ClientInteractibleComponent>();
				interactible.isEnabled = true;
			}
			else if (ClientInteractibleComponent* interactibleComponent = entity.try_get<ClientInteractibleComponent>())
			{
				// Preserve interact text if set
				if (!interactibleComponent->interactText.empty())
					interactibleComponent->isEnabled = false;
				else
					entity.remove<ClientInteractibleComponent>();
			}
		});

		entityMetatable["SetInteractibleText"] = LuaFunction([](sol::table entityTable, std::string interactibleText)
		{
			entt::handle entity = AssertScriptEntity(entityTable);

			auto& interactible = entity.get_or_emplace<ClientInteractibleComponent>();
			interactible.interactText = std::move(interactibleText);
		});
	}

	void ClientEntityScriptingLibrary::RegisterClientComponents(sol::state& state)
	{
		state.new_usertype<Nz::GraphicsComponent>("GraphicsComponent",
			sol::no_constructor,
			"AttachRenderable", LuaFunction(&Nz::GraphicsComponent::AttachRenderable)
		);

		state.new_usertype<Nz::LightComponent>("LightComponent",
			sol::no_constructor,
			"AddPointLight", LuaFunction([](Nz::LightComponent& lightComponent, sol::stack_table lightParameters)
			{
				auto& pointLight = lightComponent.AddLight<Nz::PointLight>();
				pointLight.UpdateColor(lightParameters.get_or("Color", Nz::Color::White()));
				pointLight.UpdateEnergy(lightParameters.get_or("Energy", 5.f));
			}),
			"AddSpotLight", LuaFunction([](Nz::LightComponent& lightComponent, sol::stack_table lightParameters)
			{
				auto& spotLight = lightComponent.AddLight<Nz::SpotLight>();
				spotLight.UpdateAngles(Nz::DegreeAnglef(lightParameters.get_or("InnerAngle", 45.f)), Nz::DegreeAnglef(lightParameters.get_or("OuterAngle", 60.f)));
				spotLight.UpdateColor(lightParameters.get_or("Color", Nz::Color::White()));
				spotLight.UpdateEnergy(lightParameters.get_or("Energy", 5.f));
				spotLight.UpdateRadius(lightParameters.get_or("Radius", 10.f));
			}),
			"Clear", LuaFunction(&Nz::LightComponent::Clear),
			"Hide", LuaFunction(&Nz::LightComponent::Hide),
			"Show", sol::overload(
				LuaFunction([](Nz::LightComponent& lightComponent) { lightComponent.Show(); }),
				LuaFunction(&Nz::LightComponent::Show)
			)
		);

		state.new_usertype<DistributionComponent>("DistributionComponent",
			sol::no_constructor,
			"GetInputConnectedEntity", LuaFunction([&](DistributionComponent& component, std::size_t outputIndex, sol::this_state L)
			{
				sol::state_view state(L);
				return ToEntityTable(state, component.GetInputConnectedEntity(outputIndex));
			}),
			"GetInputConnectedPort", LuaFunction(&DistributionComponent::GetInputConnectedPort),
			"GetInputCount", LuaFunction(&DistributionComponent::GetInputCount),
			"GetOutputConnectedEntity", LuaFunction([&](DistributionComponent& component, std::size_t outputIndex, sol::this_state L)
			{
				sol::state_view state(L);
				return ToEntityTable(state, component.GetOutputConnectedEntity(outputIndex));
			}),
			"GetOutputConnectedPort", LuaFunction(&DistributionComponent::GetOutputConnectedPort),
			"GetOutputCount", LuaFunction(&DistributionComponent::GetOutputCount),
			"IsInputConnected", LuaFunction(&DistributionComponent::IsInputConnected),
			"IsOutputConnected", LuaFunction(&DistributionComponent::IsOutputConnected)
		);
	}

	auto ClientEntityScriptingLibrary::RetrieveAddComponentHandler(std::string_view componentType) -> AddComponentFunc
	{
		if (AddComponentFunc addComponentHandler = SharedEntityScriptingLibrary::RetrieveAddComponentHandler(componentType))
			return addComponentHandler;

		auto it = s_clientComponents.find(componentType);
		if (it == s_clientComponents.end())
			return nullptr;

		return it->second.addComponent;
	}

	auto ClientEntityScriptingLibrary::RetrieveGetComponentHandler(std::string_view componentType) -> GetComponentFunc
	{
		if (GetComponentFunc getComponentHandler = SharedEntityScriptingLibrary::RetrieveGetComponentHandler(componentType))
			return getComponentHandler;

		auto it = s_clientComponents.find(componentType);
		if (it == s_clientComponents.end())
			return nullptr;

		return it->second.getComponent;
	}
}
