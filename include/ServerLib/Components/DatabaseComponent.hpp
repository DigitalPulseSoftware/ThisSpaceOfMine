// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_DATABASECOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_DATABASECOMPONENT_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <Nazara/Core/Uuid.hpp>
#include <optional>

namespace tsom
{
	struct DatabaseComponent
	{
		DatabaseComponent() :
		uniqueId(Nz::Uuid::Generate())
		{
		}

		DatabaseComponent(Nz::Uuid entityUniqueId) :
		uniqueId(entityUniqueId)
		{
		}

		DatabaseComponent(Nz::Uuid entityUniqueId, Nz::UInt32 id) :
		uniqueId(entityUniqueId),
		planetDatabaseId(id)
		{
		}

		Nz::Uuid uniqueId;
		std::optional<Nz::UInt32> planetDatabaseId;
	};
}

#endif // TSOM_SERVERLIB_COMPONENTS_DATABASECOMPONENT_HPP
