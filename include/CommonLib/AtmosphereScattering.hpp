// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ATMOSPHERESCATTERING_HPP
#define TSOM_COMMONLIB_ATMOSPHERESCATTERING_HPP

#include <Nazara/Math/Vector3.hpp>
#include <Nazara/Math/Vector4.hpp>
#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	enum class AtmosphereScatteringType
	{
		RoundCube,
		Torus
	};

	struct AtmosphereScattering
	{
		Nz::Vector3f sunDir = Nz::Vector3f(0.852868497f, 0.5f, 0.150383770f);
		Nz::Vector4f planetSettings = Nz::Vector4f(60.0f, 60.0f, 60.0f, 16.0f);
		float atmosphereMaxHeight = 192.f;
		AtmosphereScatteringType type = AtmosphereScatteringType::RoundCube;

		float mieScattering = 0.9f;

		float rayleighHeight = 32.f;
		float mieHeight = 5.f;
		float absorptionFalloff = 20.f;

		Nz::Int32 primarySteps = 8;
		Nz::Int32 lightSteps = 8;
	};
}

#endif // TSOM_COMMONLIB_ATMOSPHERESCATTERING_HPP
