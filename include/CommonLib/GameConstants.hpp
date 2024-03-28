// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_GAMECONSTANTS_HPP
#define TSOM_COMMONLIB_GAMECONSTANTS_HPP

#include <Nazara/Math/Angle.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom::Constants
{
	// Chat constants
	constexpr std::size_t ChatMaxLines = 100;
	constexpr std::size_t ChatMaxMessageLength = 1024;
	constexpr std::size_t ChatMaxPlayerMessageLength = 256;

	// Player constants
	constexpr std::size_t PlayerMaxNicknameLength = 16;
	constexpr float PlayerColliderRadius = 0.3f;
	constexpr float PlayerColliderHeight = 1.85f;
	constexpr float PlayerEyesHeight = 1.75f;
	constexpr float PlayerFlySpeed = 20.f;
	constexpr float PlayerJumpPower = 5.f;
	constexpr float PlayerSprintSpeed = 8.f;
	constexpr float PlayerWalkSpeed = 5.f;
	constexpr Nz::DegreeAnglef GravityMaxRotationSpeed = 180.f;
	constexpr Nz::DegreeAnglef PlayerRotationSpeed = 90.f;

	// Ship constants
	constexpr float ShipGravityAcceleration = 9.81f;

	// Computed constants
	constexpr float PlayerCapsuleHeight = PlayerColliderHeight - PlayerColliderRadius * 2.f;
	constexpr float PlayerCameraHeight = PlayerEyesHeight - PlayerColliderHeight * 0.5f;
}

#endif // TSOM_COMMONLIB_GAMECONSTANTS_HPP
