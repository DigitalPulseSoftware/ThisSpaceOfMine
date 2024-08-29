// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_GRAVITYFORCE_HPP
#define TSOM_COMMONLIB_GRAVITYFORCE_HPP

#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	struct GravityForce
	{
		Nz::Vector3f direction;
		float acceleration;
		float factor;

		static GravityForce Zero()
		{
			return { .direction = Nz::Vector3f::Zero(), .acceleration = 0.f, .factor = 0.f };
		}
	};
}

#include <CommonLib/GravityController.inl>

#endif // TSOM_COMMONLIB_GRAVITYFORCE_HPP
