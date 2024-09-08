// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientEntityScriptingLibrary.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
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
		constexpr auto s_clientComponents = frozen::make_unordered_map<frozen::string, EntityScriptingLibrary::ComponentEntry>({
			{
				"graphics", EntityScriptingLibrary::ComponentEntry::Default<Nz::GraphicsComponent>()
			}
		});
	}

	void ClientEntityScriptingLibrary::Register(sol::state& state)
	{
		EntityScriptingLibrary::Register(state);

		RegisterClientComponents(state);
	}

	void ClientEntityScriptingLibrary::FillConstants(sol::state& state, sol::table constants)
	{
		EntityScriptingLibrary::FillConstants(state, constants);

		constants["RenderMask2D"] = Constants::RenderMask2D;
		constants["RenderMaskUI"] = Constants::RenderMaskUI;
		constants["RenderMask3D"] = Constants::RenderMask3D;
		constants["RenderMaskLocalPlayer"] = Constants::RenderMaskLocalPlayer;
		constants["RenderMaskOtherPlayer"] = Constants::RenderMaskOtherPlayer;
	}

	void ClientEntityScriptingLibrary::RegisterClientComponents(sol::state& state)
	{
		state.new_usertype<Nz::GraphicsComponent>("GraphicsComponent",
			sol::no_constructor,
			"AttachRenderable", LuaFunction(&Nz::GraphicsComponent::AttachRenderable));
	}

	auto ClientEntityScriptingLibrary::RetrieveAddComponentHandler(std::string_view componentType) -> AddComponentFunc
	{
		if (AddComponentFunc addComponentHandler = EntityScriptingLibrary::RetrieveAddComponentHandler(componentType))
			return addComponentHandler;

		auto it = s_clientComponents.find(componentType);
		if (it == s_clientComponents.end())
			return nullptr;

		return it->second.addComponent;
	}

	auto ClientEntityScriptingLibrary::RetrieveGetComponentHandler(std::string_view componentType) -> GetComponentFunc
	{
		if (GetComponentFunc getComponentHandler = EntityScriptingLibrary::RetrieveGetComponentHandler(componentType))
			return getComponentHandler;

		auto it = s_clientComponents.find(componentType);
		if (it == s_clientComponents.end())
			return nullptr;

		return it->second.getComponent;
	}
}
