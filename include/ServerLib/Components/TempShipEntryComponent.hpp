// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_TEMPSHIPENTRYCOMPONENT_HPP
#define TSOM_SERVERLIB_COMPONENTS_TEMPSHIPENTRYCOMPONENT_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Math/Box.hpp>

namespace tsom
{
	class ServerShipEnvironment;

	struct TempShipEntryComponent
	{
		Nz::Boxf aabb;
		ServerShipEnvironment* shipEnv;
	};
}

#include <ServerLib/Components/TempShipEntryComponent.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_TEMPSHIPENTRYCOMPONENT_HPP
