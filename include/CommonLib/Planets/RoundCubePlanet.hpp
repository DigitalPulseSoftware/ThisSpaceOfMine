// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLANETS_ROUNDCUBEPLANET_HPP
#define TSOM_COMMONLIB_PLANETS_ROUNDCUBEPLANET_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Planet.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API RoundCubePlanet final : public Planet
	{
		public:
			inline RoundCubePlanet(Nz::ApplicationBase& app, float blockSize, Nz::UInt32 seed, float gravity, float cornerRadius);

			GravityForce ComputeGravity(const Nz::Vector3f& position) const override;
			Nz::Vector3f ComputeUpDirection(const Nz::Vector3f& position) const;

			inline float GetCornerRadius() const;
			inline float GetGravity() const;
			inline Nz::UInt32 GetSeed() const;

			inline void UpdateCornerRadius(float cornerRadius);
			inline void UpdateGravity(float gravity);

		private:
			Nz::UInt32 m_seed;
			float m_cornerRadius;
			float m_gravity;
	};
}

#include <CommonLib/Planets/RoundCubePlanet.inl>

#endif // TSOM_COMMONLIB_PLANETS_ROUNDCUBEPLANET_HPP
