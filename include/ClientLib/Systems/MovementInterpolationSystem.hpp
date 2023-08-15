// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_MOVEMENTINTERPOLATION_HPP
#define TSOM_CLIENTLIB_SYSTEMS_MOVEMENTINTERPOLATION_HPP

#include <ClientLib/Export.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <Nazara/Core/Time.hpp>
#include <entt/entt.hpp>

namespace Nz
{
	class NodeComponent;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API MovementInterpolationSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = 0;
			using Components = Nz::TypeList<class MovementInterpolationComponent, Nz::NodeComponent>;

			MovementInterpolationSystem(entt::registry& registry, Nz::Time movementTickDuration, std::size_t targetMovementPoints = 5);
			MovementInterpolationSystem(const MovementInterpolationSystem&) = delete;
			MovementInterpolationSystem(MovementInterpolationSystem&&) = default;
			~MovementInterpolationSystem() = default;

			void Update(Nz::Time elapsedTime);

			MovementInterpolationSystem& operator=(const MovementInterpolationSystem&) = delete;
			MovementInterpolationSystem& operator=(MovementInterpolationSystem&&) = default;

		private:
			entt::observer m_interpolatedObserver;
			std::size_t m_targetMovementPoints;
			Nz::Time m_movementTickDuration;
			entt::registry& m_registry;
	};
}

#include <ClientLib/Systems/MovementInterpolationSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_MOVEMENTINTERPOLATION_HPP
