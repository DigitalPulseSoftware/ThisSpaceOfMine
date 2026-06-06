// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Planets/TorusPlanet.hpp>
#include <CommonLib/Utility/SignedDistanceFunctions.hpp>
#include <Nazara/Math/Box.hpp>

namespace tsom
{
	auto TorusPlanet::ComputeGravity(const Nz::Vector3f& position) const -> GravityForce
	{
		constexpr float PlanetGravityCenterStartDecrease = 24.f;
		constexpr float PlanetGravityCenterNoGravity = 8.f;
		constexpr float PlanetGravitySpaceStart = 150.f;
		constexpr float PlanetGravitySpaceFinish = 250.f;
		constexpr float PlanetGravitySpaceNone = 500.f;

		// Decrease gravity near the center
		float distToCenterSq = position.SquaredDistance(GetCenter());
		if (distToCenterSq < Nz::IntegralPow(PlanetGravityCenterStartDecrease, 2))
		{
			float distToCenter = std::sqrt(distToCenterSq);

			Nz::Vector3f up = ComputeUpDirection(position);
			return GravityForce{
				.direction = -up,
				.acceleration = m_gravity,
				.factor = std::max(distToCenter - PlanetGravityCenterNoGravity, 0.f) / (PlanetGravityCenterStartDecrease - PlanetGravityCenterNoGravity)
			};
		}

		float dist = std::max(sdTorus(position, { m_radius, m_thickness }), 0.0f);

		// Turn torus gravity to newtonian gravity
		if (dist > PlanetGravitySpaceStart)
		{
			if (dist > PlanetGravitySpaceNone)
				return GravityForce::Zero();

			float newtonianInterp;
			if (dist > PlanetGravitySpaceFinish)
				newtonianInterp = 1.f;
			else
				newtonianInterp = std::max(dist - PlanetGravitySpaceStart, 0.f) / (PlanetGravitySpaceFinish - PlanetGravitySpaceStart);

			Nz::Vector3f direction = Nz::Vector3f::Normalize(GetCenter() - position);
			if (newtonianInterp < 0.99f)
				direction = Nz::Lerp(-ComputeUpDirection(position), direction, newtonianInterp);
			else
				direction = GetCenter() - position;

			direction.Normalize();

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

	Nz::Vector3f TorusPlanet::ComputeUpDirection(const Nz::Vector3f& position) const
	{
		// Project position on circle
		// https://stackoverflow.com/questions/6571202/closest-point-on-circle-in-3d-whats-missing

		// Project P onto the plane containing the circle.
		Nz::Vector3f positionOnCirclePlane = position - Nz::Vector3f::Up() * Nz::Vector3f::DotProduct(Nz::Vector3f::Up(), position);

		// Assume the position is not exactly at the center

		// Now the nearest point lies on the line through the origin and Q.
		Nz::Vector3f positionOnCircle = positionOnCirclePlane.Normalize() * m_radius * 0.5f;

		return Nz::Vector3f::Normalize(position - positionOnCircle);
	}
}
