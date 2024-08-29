// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <Nazara/Core/Log.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Physics3D/PhysWorld3D.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	PlanetGravitySystem::PlanetGravitySystem(entt::registry& registry, const Planet& planet, Nz::PhysWorld3D& physWorld) :
	m_registry(registry),
	m_planet(planet),
	m_physWorld(physWorld)
	{
		m_physWorld.RegisterStepListener(this);
	}

	PlanetGravitySystem::~PlanetGravitySystem()
	{
		m_physWorld.UnregisterStepListener(this);
	}

	void PlanetGravitySystem::PreSimulate(float /*elapsedTime*/)
	{
		auto view = m_registry.view<Nz::RigidBody3DComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, rigidBody] : view.each())
		{
			if (rigidBody.IsSleeping() || !rigidBody.IsDynamic())
				continue;

			Nz::Vector3f pos = rigidBody.GetPosition();
			GravityForce gravityForce = m_planet.ComputeGravity(rigidBody.GetPosition());

			rigidBody.AddForce(gravityForce.direction * gravityForce.acceleration * gravityForce.factor * rigidBody.GetMass());
		}
	}
}
