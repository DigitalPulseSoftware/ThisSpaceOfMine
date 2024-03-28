// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_GRAVITYCONTROLLER_HPP
#define TSOM_COMMONLIB_GRAVITYCONTROLLER_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API GravityController
	{
		public:
			GravityController() = default;
			GravityController(const GravityController&) = delete;
			GravityController(GravityController&&) = delete;
			virtual ~GravityController();

			virtual float ComputeGravityAcceleration(const Nz::Vector3f& position) const = 0;
			virtual Nz::Vector3f ComputeUpDirection(const Nz::Vector3f& position) const = 0;

			GravityController& operator=(const GravityController&) = delete;
			GravityController& operator=(GravityController&&) = delete;
	};
}

#include <CommonLib/GravityController.inl>

#endif // TSOM_COMMONLIB_GRAVITYCONTROLLER_HPP
