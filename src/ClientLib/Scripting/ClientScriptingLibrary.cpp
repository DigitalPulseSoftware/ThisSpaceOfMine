// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <CommonLib/ConfigFile.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Graphics/Model.hpp>
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
		RegisterConfig(state);
		RegisterScripts(state);
	}

	void ClientScriptingLibrary::RegisterClientSession(sol::state& state)
	{
		sol::table sessionLibrary = state.create_named_table("ClientSession");

	}

	void ClientScriptingLibrary::RegisterConfig(sol::state& state)
	{
		sol::table configLibrary = state.create_named_table("Config");
		configLibrary["GetBool"] = LuaFunction([this](std::string_view configName)
		{
			return m_config.GetBoolValue(ConfigFile::BoolOptionName{ configName });
		});

		configLibrary["GetFloat"] = LuaFunction([this](std::string_view configName)
		{
			return m_config.GetFloatValue<lua_Number>(ConfigFile::FloatOptionName{ configName });
		});

		configLibrary["GetInteger"] = LuaFunction([this](std::string_view configName)
		{
			return m_config.GetIntegerValue<lua_Integer>(ConfigFile::IntegerOptionName{ configName });
		});

		configLibrary["GetString"] = LuaFunction([this](std::string_view configName)
		{
			return m_config.GetStringValue(ConfigFile::StringOptionName{ configName });
		});

		configLibrary["SetBool"] = LuaFunction([this](std::string_view configName, bool value)
		{
			return m_config.SetBoolValue(ConfigFile::BoolOptionName{ configName }, value);
		});

		configLibrary["SetFloat"] = LuaFunction([this](std::string_view configName, double value)
		{
			return m_config.SetFloatValue(ConfigFile::FloatOptionName{ configName }, value);
		});

		configLibrary["SetInteger"] = LuaFunction([this](std::string_view configName, long long value)
		{
			return m_config.SetIntegerValue(ConfigFile::IntegerOptionName{ configName }, value);
		});

		configLibrary["SetString"] = LuaFunction([this](std::string_view configName, std::string value)
		{
			return m_config.SetStringValue(ConfigFile::StringOptionName{ configName }, std::move(value));
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
