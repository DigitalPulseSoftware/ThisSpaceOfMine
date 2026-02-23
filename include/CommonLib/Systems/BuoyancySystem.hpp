// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_BUOYANCYSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_BUOYANCYSYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Physics3D/PhysWorld3DStepListener.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class PhysBody3D;
	class PhysWorld3D;
	struct PhysContact3D;
	struct PhysContactResponse3D;
}

namespace tsom
{
	class DebugDrawInterface;
	class GravityController;

	class TSOM_COMMONLIB_API BuoyancySystem : public Nz::PhysWorld3DStepListener
	{
		public:
			BuoyancySystem(entt::registry& registry, const GravityController& gravityController, Nz::PhysWorld3D& physWorld, DebugDrawInterface* debugDrawInterface = nullptr);
			BuoyancySystem(const BuoyancySystem&) = delete;
			BuoyancySystem(BuoyancySystem&&) = delete;
			~BuoyancySystem();

			void PreSimulate(float elapsedTime) override;

			BuoyancySystem& operator=(const BuoyancySystem&) = delete;
			BuoyancySystem& operator=(BuoyancySystem&&) = delete;

			static void HandleContactAdded(entt::handle entity, const Nz::PhysBody3D* body, entt::handle otherEntity, const Nz::PhysBody3D* otherBody, const Nz::PhysContact3D& physContact, Nz::PhysContactResponse3D& physContactResponse);
			static void HandleContactPersisted(entt::handle entity, const Nz::PhysBody3D* body, entt::handle otherEntity, const Nz::PhysBody3D* otherBody, const Nz::PhysContact3D& physContact, Nz::PhysContactResponse3D& physContactResponse);
			static void HandleContactRemoved(entt::handle entity1, Nz::UInt32 body1Index, const Nz::PhysBody3D* body1, Nz::UInt32 subShapeID1, entt::handle entity2, Nz::UInt32 body2Index, const Nz::PhysBody3D* body2, Nz::UInt32 subShapeID2);

		private:
			entt::registry& m_registry;
			Nz::PhysWorld3D& m_physWorld;
			DebugDrawInterface* m_debugDrawInterface;
			const GravityController& m_gravityController;
	};
}

#include <CommonLib/Systems/BuoyancySystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_BUOYANCYSYSTEM_HPP
