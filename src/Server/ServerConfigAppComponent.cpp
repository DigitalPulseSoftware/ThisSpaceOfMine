// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Server/ServerConfigAppComponent.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	ServerConfigFile::ServerConfigFile()
	{
		RegisterStringOption("Api.Url");
		RegisterStringOption("ConnectionToken.EncryptionKey", "");
		RegisterIntegerOption("Server.Port", 1, 0xFFFF, 29536);
		RegisterIntegerOption("Server.MaxStuckSeconds", 0, 60, 10);
		RegisterBoolOption("Server.SleepWhenEmpty", true);
		RegisterStringOption("Save.Directory", "saves/chunks");
		RegisterIntegerOption("Save.Interval", 0, 60 * 60, 30);
	}

	void ServerConfigFile::PostLoad()
	{
		using base64 = cppcodec::base64_rfc4648;

		std::vector<std::uint8_t> connectionTokenEncryptionKey = base64::decode(GetStringValue("ConnectionToken.EncryptionKey"));
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
			fmt::print(fg(fmt::color::red), "failed to load server config\n");
			return;
		}

		try
		{
			m_configFile.PostLoad();
		}
		catch (const std::exception& e)
		{
			fmt::print(fg(fmt::color::red), "failed to load server config: {0}\n", e.what());
			return;
		}
	}

	void ServerConfigAppComponent::Save()
	{
		if (!m_configFile.SaveToFile(Nz::Utf8Path("serverconfig.lua")))
			fmt::print(fg(fmt::color::red), "failed to save server config\n");
	}
}
