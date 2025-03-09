// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <fmt/format.h>
#include <sol/state.hpp>

SOL_BASE_CLASSES(Nz::Model, Nz::InstancedRenderable);
SOL_DERIVED_CLASSES(Nz::InstancedRenderable, Nz::Model);

namespace sol
{
	// Don't treat Nz::Flags as containers (despite having begin() and end())
	template<typename E>
	struct is_container<Nz::Flags<E>> : std::false_type {};

	// Not sure why it's required for sol::var_wrapper as well
	template<typename E>
	struct is_container<sol::var_wrapper<Nz::Flags<E>>> : std::false_type {};
}

namespace tsom
{
	void ClientScriptingLibrary::Register(sol::state& state)
	{
		state["CLIENT"] = true;
		state["SERVER"] = false;

		RegisterClientSession(state);
		RegisterScripts(state);
	}


	void ClientScriptingLibrary::RegisterClientSession(sol::state& state)
	{
		sol::table sessionLibrary = state.create_named_table("ClientSession");

		sessionLibrary["EnableShipControl"] = LuaFunction([this](bool enable)
		{
			m_sessionHandler.EnableShipControl(enable);
		});
	}

	void ClientScriptingLibrary::RegisterScripts(sol::state& state)
	{
		sol::table scriptsLibrary = state.create_named_table("Scripts");

		scriptsLibrary["Reload"] = LuaFunction([this]
		{
			m_sessionHandler.LoadScripts(true);
		});
	}
}
