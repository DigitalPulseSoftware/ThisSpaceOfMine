// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVER_SERVERCONFIGAPPCOMPONENT_HPP
#define TSOM_SERVER_SERVERCONFIGAPPCOMPONENT_HPP

#include <CommonLib/ConfigFile.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	class ServerConfigFile : public ConfigFile
	{
		public:
			ServerConfigFile();
	};

	class ServerConfigAppComponent final : public Nz::ApplicationComponent
	{
		public:
			ServerConfigAppComponent(Nz::ApplicationBase& app);
			ServerConfigAppComponent(const ServerConfigAppComponent&) = delete;
			ServerConfigAppComponent(ServerConfigAppComponent&&) = delete;
			~ServerConfigAppComponent() = default;

			inline ServerConfigFile& GetConfig();
			inline const ServerConfigFile& GetConfig() const;

			void Save();

			ServerConfigAppComponent& operator=(const ServerConfigAppComponent&) = delete;
			ServerConfigAppComponent& operator=(ServerConfigAppComponent&&) = delete;

		private:
			ServerConfigFile m_configFile;
	};
}

#include <Server/ServerConfigAppComponent.inl>

#endif // TSOM_SERVER_SERVERCONFIGAPPCOMPONENT_HPP
