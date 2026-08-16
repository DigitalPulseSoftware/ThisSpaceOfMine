// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline TorusPlanet::TorusPlanet(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary, float blockSize, Nz::UInt32 seed, float gravity, float radius, float thickness) :
	Planet(app, blockLibrary, blockSize),
	m_seed(seed),
	m_gravity(gravity),
	m_radius(radius),
	m_thickness(thickness)
	{
	}

	inline float TorusPlanet::GetGravity() const
	{
		return m_gravity;
	}

	inline float TorusPlanet::GetRadius() const
	{
		return m_radius;
	}

	inline float TorusPlanet::GetThickness() const
	{
		return m_thickness;
	}

	inline Nz::UInt32 TorusPlanet::GetSeed() const
	{
		return m_seed;
	}

	inline void TorusPlanet::UpdateGravity(float gravity)
	{
		m_gravity = gravity;
	}

	inline void TorusPlanet::UpdateRadius(float radius)
	{
		m_radius = radius;
	}

	inline void TorusPlanet::UpdateThickness(float thickness)
	{
		m_thickness = thickness;
	}
}
