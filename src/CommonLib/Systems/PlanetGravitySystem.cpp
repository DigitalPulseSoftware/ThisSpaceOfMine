// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Components/PlanetGravityComponent.hpp>
#include <Nazara/Core/Log.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void PlanetGravitySystem::PreSimulate(float /*elapsedTime*/)
	{
		auto view = m_registry.view<Nz::RigidBody3DComponent, PlanetGravityComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, rigidBody, planetComponent] : view.each())
		{
			if (rigidBody.IsSleeping() || !planetComponent.planet)
				continue;

			Nz::Vector3f pos = rigidBody.GetPosition();
			Nz::Vector3f up = planetComponent.planet->ComputeUpDirection(rigidBody.GetPosition());
			rigidBody.AddForce(-up * planetComponent.planet->GetGravityFactor(pos) * rigidBody.GetMass());
		}
	}
}
