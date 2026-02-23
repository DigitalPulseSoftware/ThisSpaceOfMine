// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Server/ServerConfigAppComponent.hpp>
#include <CommonLib/CommonConfigs.hpp>
#include <Server/ServerConfigs.hpp>
#include <Nazara/Core/StringExt.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ServerConfigFile::ServerConfigFile()
	{
		RegisterStringOption(Config::Api_Url);
		RegisterStringOption(Config::ConnectionToken_EncryptionKey, "");
		RegisterIntegerOption(Config::Server_Port, 1, 0xFFFF, 29536);
		RegisterIntegerOption(Config::Server_MaxStuckSeconds, 0, 60, 10);
		RegisterBoolOption(Config::Debug_EnableDrawer, true);
		RegisterBoolOption(Config::Server_SleepWhenEmpty, true);
		RegisterStringOption(Config::Save_Directory, "saves/chunks");
		RegisterStringOption(Config::Database_Filename, "server_database.db");
		RegisterIntegerOption(Config::Save_Interval, 0, 60 * 60, 30);

		// Server.AutoUpdater
		RegisterBoolOption(Config::Server_AutoUpdater_Enabled, false);
		RegisterIntegerOption(Config::Server_AutoUpdater_CheckInterval, 1, 60 * 60, 30);
		RegisterIntegerOption(Config::Server_AutoUpdater_QuitDelay, 1, 60 * 60, 10 * 60);
		RegisterStringOption(Config::Server_AutoUpdater_Behavior, "downloadandupdate", [](std::string value) -> Nz::Result<std::string, std::string>
		{
			value = Nz::ToLower(value);
			if (value != "downloadandupdate" && value != "downloadandexit")
				return Nz::Err(fmt::format("unknown value {}, possible values are DownloadAndUpdate or DownloadAndExit", value));

			return Nz::Ok(value);
		});
	}

	void ServerConfigFile::PostLoad()
	{
		using base64 = cppcodec::base64_rfc4648;

		std::vector<std::uint8_t> connectionTokenEncryptionKey = base64::decode(GetStringValue(Config::ConnectionToken_EncryptionKey));
		if (!connectionTokenEncryptionKey.empty())
		{
			if (connectionTokenEncryptionKey.size() != m_connectionTokenEncryptionKey.size())
				throw std::runtime_error(fmt::format("connection token encryption key has incorrect length (expected 32bytes, got {0})", connectionTokenEncryptionKey.size()));

			std::memcpy(&m_connectionTokenEncryptionKey[0], &connectionTokenEncryptionKey[0], m_connectionTokenEncryptionKey.size());
		}
	}


	ServerConfigAppComponent::ServerConfigAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app)
	{
		std::filesystem::path configPath = Nz::Utf8Path("serverconfig.lua");
		std::filesystem::path defaultConfigPath = configPath;
		defaultConfigPath.replace_extension(Nz::Utf8Path(".lua.default"));

		if (!std::filesystem::is_regular_file(configPath) && std::filesystem::is_regular_file(defaultConfigPath))
			configPath = std::move(defaultConfigPath);

		if (!m_configFile.LoadFromFile(configPath))
		{
			spdlog::error("failed to load server config");
			return;
		}

		try
		{
			m_configFile.PostLoad();
		}
		catch (const std::exception& e)
		{
			spdlog::error("failed to load server config: {0}", e.what());
			return;
		}
	}

	void ServerConfigAppComponent::Save()
	{
		if (!m_configFile.SaveToFile(Nz::Utf8Path("serverconfig.lua")))
			spdlog::error("failed to save server config");
	}
}
