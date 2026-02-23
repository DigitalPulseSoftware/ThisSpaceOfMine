// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVER_SERVERCONFIGS_HPP
#define TSOM_SERVER_SERVERCONFIGS_HPP

#include <CommonLib/ConfigFile.hpp>

namespace tsom::Config
{
	static constexpr auto ConnectionToken_EncryptionKey = ConfigFile::StringOptionName{ "ConnectionToken.EncryptionKey" };
	static constexpr auto Database_Filename = ConfigFile::StringOptionName{ "Database.Filename" };
	static constexpr auto Debug_EnableDrawer = ConfigFile::BoolOptionName{ "Debug.EnableDrawer" };
	static constexpr auto Save_Directory = ConfigFile::StringOptionName{ "Save.Directory" };
	static constexpr auto Save_Interval = ConfigFile::IntegerOptionName{ "Save.Interval" };
	static constexpr auto Server_MaxStuckSeconds = ConfigFile::IntegerOptionName{ "Server.MaxStuckSeconds" };
	static constexpr auto Server_Port = ConfigFile::IntegerOptionName{ "Server.Port" };
	static constexpr auto Server_SleepWhenEmpty = ConfigFile::BoolOptionName{ "Server.SleepWhenEmpty" };

	// Server.AutoUpdater
	static constexpr auto Server_AutoUpdater_Enabled = ConfigFile::BoolOptionName{ "Server.AutoUpdater.Enabled" };
	static constexpr auto Server_AutoUpdater_CheckInterval = ConfigFile::IntegerOptionName{ "Server.AutoUpdater.CheckInterval" };
	static constexpr auto Server_AutoUpdater_QuitDelay = ConfigFile::IntegerOptionName{ "Server.AutoUpdater.QuitDelay" };
	static constexpr auto Server_AutoUpdater_Behavior = ConfigFile::StringOptionName{ "Server.AutoUpdater.Behavior" };
}

#endif // TSOM_SERVER_SERVERCONFIGS_HPP
