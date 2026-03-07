// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerConfig.hpp>
#include <CommonLib/Utility/JsonSerialization.hpp>
#include <ServerLib/Database/ServerDatabase.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ServerConfig ServerConfig::Load(const ServerDatabase& database)
	{
		ServerConfig serverConfig;

		database.GetAllConfigs([&](Database::Config&& config)
		{
			if (config.name == "default_spawnpoint")
			{
				serverConfig.defaultSpawnpoint.planetId = config.value["planet_id"];
				serverConfig.defaultSpawnpoint.position = config.value["position"];
				serverConfig.defaultSpawnpoint.rotation = config.value.value("rotation", Nz::Quaternionf::Identity());
			}
			else
				spdlog::warn("unknown database config \"{}\"", config.name);

			return true;
		});

		return serverConfig;
	}
}
