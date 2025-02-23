// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_ATMOSPHEREMONITOR_HPP
#define TSOM_SERVERLIB_COMPONENTS_ATMOSPHEREMONITOR_HPP

namespace tsom
{
	class ServerAtmosphere;

	struct AtmosphereMonitor
	{
		ServerAtmosphere* atmosphere = nullptr;
	};
}

#include <ServerLib/Components/AtmosphereMonitor.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_ATMOSPHEREMONITOR_HPP
