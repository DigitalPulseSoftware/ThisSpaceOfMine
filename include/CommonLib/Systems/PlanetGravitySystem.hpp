// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_MOVEMENTINTERPOLATION_HPP
#define TSOM_COMMONLIB_SYSTEMS_MOVEMENTINTERPOLATION_HPP

#include <CommonLib/Export.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/JoltPhysics3D/JoltPhysicsStepListener.hpp>
#include <entt/fwd.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API PlanetGravitySystem : public Nz::JoltPhysicsStepListener
	{
		public:
			inline PlanetGravitySystem(entt::registry& registry);
			PlanetGravitySystem(const PlanetGravitySystem&) = delete;
			PlanetGravitySystem(PlanetGravitySystem&&) = default;
			~PlanetGravitySystem() = default;

			void PreSimulate(float elapsedTime);

			PlanetGravitySystem& operator=(const PlanetGravitySystem&) = delete;
			PlanetGravitySystem& operator=(PlanetGravitySystem&&) = default;

		private:
			entt::registry& m_registry;
	};
}

#include <CommonLib/Systems/PlanetGravitySystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_MOVEMENTINTERPOLATION_HPP
