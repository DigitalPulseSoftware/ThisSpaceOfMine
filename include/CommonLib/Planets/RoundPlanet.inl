// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline RoundPlanet::RoundPlanet(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary, float blockSize, float radius, Nz::UInt32 seed, float gravity) :
	Planet(app, blockLibrary, blockSize, radius),
	m_seed(seed),
	m_gravity(gravity)
	{
	}

	inline float RoundPlanet::GetGravity() const
	{
		return m_gravity;
	}

	inline Nz::UInt32 RoundPlanet::GetSeed() const
	{
		return m_seed;
	}

	inline void RoundPlanet::UpdateGravity(float gravity)
	{
		m_gravity = gravity;
	}
}
