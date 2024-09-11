// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_GRAVITYPHYSICSSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_GRAVITYPHYSICSSYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Physics3D/PhysWorld3DStepListener.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class PhysWorld3D;
}

namespace tsom
{
	class GravityController;

	class TSOM_COMMONLIB_API GravityPhysicsSystem : public Nz::PhysWorld3DStepListener
	{
		public:
			GravityPhysicsSystem(entt::registry& registry, const GravityController& gravityController, Nz::PhysWorld3D& physWorld);
			GravityPhysicsSystem(const GravityPhysicsSystem&) = delete;
			GravityPhysicsSystem(GravityPhysicsSystem&&) = delete;
			~GravityPhysicsSystem();

			void PreSimulate(float elapsedTime) override;

			GravityPhysicsSystem& operator=(const GravityPhysicsSystem&) = delete;
			GravityPhysicsSystem& operator=(GravityPhysicsSystem&&) = delete;

		private:
			entt::registry& m_registry;
			const GravityController& m_gravityController;
			Nz::PhysWorld3D& m_physWorld;
	};
}

#include <CommonLib/Systems/GravityPhysicsSystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_GRAVITYPHYSICSSYSTEM_HPP
