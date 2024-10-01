// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLAYERINPUTS_HPP
#define TSOM_COMMONLIB_PLAYERINPUTS_HPP

#include <CommonLib/InputIndex.hpp>
#include <Nazara/Math/Angle.hpp>
#include <variant>

namespace tsom
{
	struct PlayerInputs
	{
		InputIndex index;

		struct Character
		{
			bool crouch = false;
			bool jump = false;
			bool moveForward = false;
			bool moveBackward = false;
			bool moveLeft = false;
			bool moveRight = false;
			bool sprint = false;
			Nz::RadianAnglef pitch = 0.f;
			Nz::RadianAnglef yaw = 0.f;
		};

		struct Ship
		{
			bool moveForward = false;
			bool moveBackward = false;
			bool moveLeft = false;
			bool moveRight = false;
			bool rollLeft = false;
			bool rollRight = false;
			Nz::RadianAnglef pitch = 0.f;
			Nz::RadianAnglef yaw = 0.f;
		};

		std::variant<std::monostate, Character, Ship> data;
	};
}

#endif // TSOM_COMMONLIB_PLAYERINPUTS_HPP
