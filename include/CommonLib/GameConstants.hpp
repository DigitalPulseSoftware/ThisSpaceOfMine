// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_GAMECONSTANTS_HPP
#define TSOM_COMMONLIB_GAMECONSTANTS_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Math/Angle.hpp>

namespace tsom::Constants
{
	constexpr Nz::Time TickDuration = Nz::Time::TickDuration(60);
	constexpr Nz::UInt16 ServerPort = 29536;

	// Player constants
	constexpr float PlayerColliderRadius = 0.3f;
	constexpr float PlayerColliderHeight = 1.8f - PlayerColliderRadius * 2.f;
	constexpr float PlayerJumpPower = 5.f;
	constexpr float PlayerSprintSpeed = 8.f;
	constexpr float PlayerWalkSpeed = 5.f;
	constexpr Nz::DegreeAnglef GravityMaxRotationSpeed = 180.f;
	constexpr Nz::DegreeAnglef PlayerRotationSpeed = 90.f;
}

#endif
