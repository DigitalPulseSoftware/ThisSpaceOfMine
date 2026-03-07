// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline PhysicsInterpolationSystem::PhysicsInterpolationSystem(entt::registry& registry, Nz::Physics3DSystem& physicsSystem) :
	m_registry(registry),
	m_physicsSystem(physicsSystem),
	m_prevAccumulator(Nz::Time::Zero())
	{
	}
}
