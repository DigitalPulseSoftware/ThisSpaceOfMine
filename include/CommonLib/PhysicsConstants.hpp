// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PHYSICSCONSTANTS_HPP
#define TSOM_COMMONLIB_PHYSICSCONSTANTS_HPP

#include <Nazara/Physics3D/PhysObjectLayer3D.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom::Constants
{
	// Broadphase layers
	static constexpr Nz::PhysBroadphase3D BroadphaseStatic = 0;
	static constexpr Nz::PhysBroadphase3D BroadphaseDynamic = 1;

	// Object layers
	static constexpr Nz::PhysObjectLayer3D ObjectLayerDynamic = 2;
	static constexpr Nz::PhysObjectLayer3D ObjectLayerDynamicNoPlayer = 4;
	static constexpr Nz::PhysObjectLayer3D ObjectLayerDynamicTrigger = 6;
	static constexpr Nz::PhysObjectLayer3D ObjectLayerPlayer = 3;
	static constexpr Nz::PhysObjectLayer3D ObjectLayerStatic = 0;
	static constexpr Nz::PhysObjectLayer3D ObjectLayerStaticNoPlayer = 1;
	static constexpr Nz::PhysObjectLayer3D ObjectLayerStaticTrigger = 5;
}

#endif // TSOM_COMMONLIB_PHYSICSCONSTANTS_HPP
