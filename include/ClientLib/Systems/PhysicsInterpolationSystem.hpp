// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_PHYSICSINTERPOLATIONSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_PHYSICSINTERPOLATIONSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class NodeComponent;
	class Physics3DSystem;
	class RigidBody3DComponent;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API PhysicsInterpolationSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = 1; //< after physics
			using Components = Nz::TypeList<class PhysicsInterpolationComponent, Nz::NodeComponent>;

			inline PhysicsInterpolationSystem(entt::registry& registry, Nz::Physics3DSystem& physicsSystem);
			PhysicsInterpolationSystem(const PhysicsInterpolationSystem&) = delete;
			PhysicsInterpolationSystem(PhysicsInterpolationSystem&&) = delete;
			~PhysicsInterpolationSystem() = default;

			void Update(Nz::Time elapsedTime);

			PhysicsInterpolationSystem& operator=(const PhysicsInterpolationSystem&) = delete;
			PhysicsInterpolationSystem& operator=(PhysicsInterpolationSystem&&) = delete;

		private:
			entt::registry& m_registry;
			Nz::Physics3DSystem& m_physicsSystem;
			Nz::Time m_prevAccumulator;
	};
}

#include <ClientLib/Systems/PhysicsInterpolationSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_PHYSICSINTERPOLATIONSYSTEM_HPP
