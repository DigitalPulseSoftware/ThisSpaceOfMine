// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PHYSICS_PHYSICSSETTINGS_HPP
#define TSOM_COMMONLIB_PHYSICS_PHYSICSSETTINGS_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom::Physics
{
	TSOM_COMMONLIB_API Nz::Physics3DSystem::Settings BuildSettings();
}

#include <CommonLib/Physics/PhysicsSettings.inl>

#endif // TSOM_COMMONLIB_PHYSICS_PHYSICSSETTINGS_HPP
