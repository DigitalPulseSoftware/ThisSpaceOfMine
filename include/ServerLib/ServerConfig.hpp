// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERCONFIG_HPP
#define TSOM_SERVERLIB_SERVERCONFIG_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	class ServerDatabase;
	class ServerEnvironment;

	struct TSOM_SERVERLIB_API ServerConfig
	{
		struct Spawnpoint
		{
			Nz::UInt32 planetId;
			Nz::Quaternionf rotation;
			Nz::Vector3f position;
		};

		Spawnpoint defaultSpawnpoint;

		static ServerConfig Load(const ServerDatabase& database);
	};
}

#include <ServerLib/ServerConfig.inl>

#endif // TSOM_SERVERLIB_SERVERCONFIG_HPP
