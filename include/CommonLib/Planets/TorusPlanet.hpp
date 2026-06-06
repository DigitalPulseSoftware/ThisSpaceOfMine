// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLANETS_TORUSPLANET_HPP
#define TSOM_COMMONLIB_PLANETS_TORUSPLANET_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Planet.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API TorusPlanet final : public Planet
	{
		public:
			inline TorusPlanet(Nz::ApplicationBase& app, float blockSize, Nz::UInt32 seed, float gravity, float radius, float thickness);

			GravityForce ComputeGravity(const Nz::Vector3f& position) const override;
			Nz::Vector3f ComputeUpDirection(const Nz::Vector3f& position) const;

			inline float GetGravity() const;
			inline float GetRadius() const;
			inline Nz::UInt32 GetSeed() const;
			inline float GetThickness() const;

			inline void UpdateGravity(float gravity);
			inline void UpdateRadius(float radius);
			inline void UpdateThickness(float thickness);

		private:
			Nz::UInt32 m_seed;
			float m_gravity;
			float m_radius;
			float m_thickness;
	};
}

#include <CommonLib/Planets/TorusPlanet.inl>

#endif // TSOM_COMMONLIB_PLANETS_TORUSPLANET_HPP
