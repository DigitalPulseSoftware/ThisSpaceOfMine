// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_PLANETGRAVITYSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_PLANETGRAVITYSYSTEM_HPP

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
	class TSOM_COMMONLIB_API PlanetGravitySystem : public Nz::PhysWorld3DStepListener
	{
		public:
			PlanetGravitySystem(entt::registry& registry, Nz::PhysWorld3D& physWorld);
			PlanetGravitySystem(const PlanetGravitySystem&) = delete;
			PlanetGravitySystem(PlanetGravitySystem&&) = delete;
			~PlanetGravitySystem();

			void PreSimulate(float elapsedTime) override;

			PlanetGravitySystem& operator=(const PlanetGravitySystem&) = delete;
			PlanetGravitySystem& operator=(PlanetGravitySystem&&) = delete;

		private:
			entt::registry& m_registry;
			Nz::PhysWorld3D& m_physWorld;
	};
}

#include <CommonLib/Systems/PlanetGravitySystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_PLANETGRAVITYSYSTEM_HPP
