// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_DATABASE_SCHEMA_HPP
#define TSOM_SERVERLIB_DATABASE_SCHEMA_HPP

#include <Nazara/Core/Uuid.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <nlohmann/json.hpp>
#include <span>
#include <string_view>

namespace tsom::Database
{
	struct Planet
	{
		Nz::UInt32 id;
		std::string_view generatorName;
		Nz::UInt32 seed;
		Nz::Vector3ui32 chunkCount;
		float cornerRadius;
		float gravity;
	};

	struct PlanetChunk
	{
		Nz::UInt32 planetId;
		Nz::Vector3i32 position;
		Nz::UInt32 version;
		std::span<const Nz::UInt8> chunkData;
	};

	struct PlanetEntityPartial
	{
		Nz::UInt32 classVersion;
		Nz::Vector3f position;
		Nz::Quaternionf rotation;
		nlohmann::json properties;
	};

	struct PlanetEntity : PlanetEntityPartial
	{
		Nz::UInt32 id;
		Nz::Uuid uniqueId;
		Nz::UInt32 planetId;
		std::string_view className;
	};

	struct PlanetLink
	{
		Nz::UInt32 sourcePlanet;
		Nz::UInt32 destinationPlanet;
		Nz::Vector3f position;
	};
}

#include <ServerLib/Database/Schema.inl>

#endif // TSOM_SERVERLIB_DATABASE_SCHEMA_HPP
