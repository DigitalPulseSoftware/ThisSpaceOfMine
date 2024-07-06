// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Server/ServerConfigAppComponent.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	ServerConfigFile::ServerConfigFile()
	{
		RegisterStringOption("Api.Url");
		RegisterIntegerOption("Server.Port", 1, 0xFFFF, 29536);
		RegisterBoolOption("Server.SleepWhenEmpty", true);
		RegisterStringOption("Save.Directory", "saves/chunks");
		RegisterIntegerOption("Save.Interval", 0, 60 * 60, 30);
	}

	ServerConfigAppComponent::ServerConfigAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app)
	{
		if (!m_configFile.LoadFromFile(Nz::Utf8Path("serverconfig.lua")))
			fmt::print(fg(fmt::color::red), "failed to load server config\n");
	}

	void ServerConfigAppComponent::Save()
	{
		if (!m_configFile.SaveToFile(Nz::Utf8Path("serverconfig.lua")))
			fmt::print(fg(fmt::color::red), "failed to save server config\n");
	}
}
