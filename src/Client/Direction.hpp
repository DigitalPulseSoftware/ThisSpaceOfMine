// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_DIRECTION_HPP
#define TSOM_CLIENT_DIRECTION_HPP

namespace tsom
{
	enum class Direction
	{
		Back,
		Down,
		Front,
		Left,
		Right,
		Up,

		Max = Up
	};
}

#endif // TSOM_CLIENT_DIRECTION_HPP
