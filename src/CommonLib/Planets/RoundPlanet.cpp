// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Planets/RoundPlanet.hpp>

namespace tsom
{
	auto RoundPlanet::ComputeGravity(const Nz::Vector3f& position) const -> GravityForce
	{
		constexpr float PlanetGravityCenterStartDecrease = 24.f;
		constexpr float PlanetGravityCenterNoGravity = 8.f;
		constexpr float PlanetGravitySpaceStart = 150.f;
		constexpr float PlanetGravitySpaceFinish = 250.f;
		constexpr float PlanetGravitySpaceNone = 500.f;

		// Decrease gravity near the center
		float distSq = position.SquaredDistance(GetCenter());
		if (distSq < Nz::IntegralPow(PlanetGravityCenterStartDecrease, 2))
		{
			Nz::Vector3f up = ComputeUpDirection(position);
			return GravityForce{
				.direction = -up,
				.acceleration = m_gravity,
				.factor = std::max(std::sqrt(distSq) - PlanetGravityCenterNoGravity, 0.f) / (PlanetGravityCenterStartDecrease - PlanetGravityCenterNoGravity)
			};
		}

		// Decrease gravity starting from a distance
		if (distSq > Nz::IntegralPow(PlanetGravitySpaceStart, 2))
		{
			if (distSq > Nz::IntegralPow(PlanetGravitySpaceNone, 2))
				return GravityForce::Zero();

			float dist = std::sqrt(distSq);
			Nz::Vector3f direction = Nz::Vector3f::Normalize(GetCenter() - position);

			float gravity = std::max(dist - PlanetGravitySpaceFinish, 0.f) / (PlanetGravitySpaceNone - PlanetGravitySpaceFinish);
			gravity *= gravity;

			return GravityForce{
				.direction = direction,
				.acceleration = m_gravity,
				.factor = 1.f - gravity
			};
		}

		// Regular gravity
		Nz::Vector3f up = ComputeUpDirection(position);
		return GravityForce{
			.direction = -up,
			.acceleration = m_gravity,
			.factor = 1.f
		};
	}

	Nz::Vector3f RoundPlanet::ComputeUpDirection(const Nz::Vector3f& position) const
	{
		Nz::Vector3f center = GetCenter();
		return Nz::Vector3f::Normalize(position - center);
	}
}
