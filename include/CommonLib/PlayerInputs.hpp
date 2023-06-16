// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_PLAYERINPUTS_HPP
#define TSOM_COMMONLIB_PLAYERINPUTS_HPP

namespace tsom
{
	struct PlayerInputs
	{
		bool jump = false;
		bool moveForward = false;
		bool moveBackward = false;
		bool moveLeft = false;
		bool moveRight = false;
		bool sprint = false;
	};
}

#endif // TSOM_COMMONLIB_PLAYERINPUTS_HPP
