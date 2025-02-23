// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_COMPONENTS_ATMOSPHERECARRIER_HPP
#define TSOM_SERVERLIB_COMPONENTS_ATMOSPHERECARRIER_HPP

#include <Nazara/Physics3D/Collider3D.hpp>

namespace tsom
{
	class ServerAtmosphere;

	struct AtmosphereCarrier
	{
		std::shared_ptr<Nz::Collider3D> collider;
		Nz::Boxf aabb; //< in trigger space, serves as a cheap test before testing entryTrigger
		ServerAtmosphere* atmosphere;
	};
}

#include <ServerLib/Components/AtmosphereCarrier.inl>

#endif // TSOM_SERVERLIB_COMPONENTS_ATMOSPHERECARRIER_HPP
