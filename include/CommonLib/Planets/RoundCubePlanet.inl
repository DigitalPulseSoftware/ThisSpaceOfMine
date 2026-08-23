// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline RoundCubePlanet::RoundCubePlanet(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary, float blockSize, Nz::UInt32 seed, float gravity, float cornerRadius) :
	Planet(app, blockLibrary, blockSize),
	m_seed(seed),
	m_cornerRadius(cornerRadius),
	m_gravity(gravity)
	{
	}

	inline float RoundCubePlanet::GetCornerRadius() const
	{
		return m_cornerRadius;
	}

	inline float RoundCubePlanet::GetGravity() const
	{
		return m_gravity;
	}

	inline Nz::UInt32 RoundCubePlanet::GetSeed() const
	{
		return m_seed;
	}

	inline void RoundCubePlanet::UpdateCornerRadius(float cornerRadius)
	{
		m_cornerRadius = cornerRadius;
	}

	inline void tsom::RoundCubePlanet::UpdateGravity(float gravity)
	{
		m_gravity = gravity;
	}
}
