// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_GAMECONFIGS_HPP
#define TSOM_GAME_GAMECONFIGS_HPP

#include <CommonLib/ConfigFile.hpp>

namespace tsom::Config
{
	static constexpr auto Api_DevMode = ConfigFile::BoolOptionName{ "Api.DevMode" };
	static constexpr auto Menu_Login = ConfigFile::StringOptionName{ "Menu.Login" };
	static constexpr auto Menu_ServerAddress = ConfigFile::StringOptionName{ "Menu.ServerAddress" };
	static constexpr auto Input_MouseSensitivity = ConfigFile::FloatOptionName{ "Input.MouseSensitivity" };
	static constexpr auto Player_Token = ConfigFile::StringOptionName{ "Player.Token" };
	static constexpr auto Server_Port = ConfigFile::IntegerOptionName{ "Server.Port" };
	static constexpr auto Graphics_FPSLimit = ConfigFile::IntegerOptionName{ "Graphics.FPSLimit" };
}

#endif // TSOM_GAME_GAMECONFIGS_HPP
