// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/GameConfigAppComponent.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	GameConfigFile::GameConfigFile()
	{
		RegisterStringOption("Menu.Login", "Mingebag", [](std::string value) -> Nz::Result<std::string, std::string>
		{
			if (value.empty())
				return Nz::Err("name cannot be empty");

			if (value.size() > 32)
				return Nz::Err("name is too long");

			return Nz::Ok(std::move(value));
		});

		RegisterStringOption("Menu.ServerAddress", "tsom.digitalpulse.software");
		RegisterIntegerOption("Server.Port", 1, 0xFFFF, 29536);
	}

	GameConfigAppComponent::GameConfigAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app)
	{
		if (!m_configFile.LoadFromFile(Nz::Utf8Path("gameconfig.lua")))
			fmt::print(fg(fmt::color::red), "failed to load game config\n");
	}

	void GameConfigAppComponent::Save()
	{
		if (!m_configFile.SaveToFile(Nz::Utf8Path("gameconfig.lua")))
			fmt::print(fg(fmt::color::red), "failed to save game config\n");
	}
}
