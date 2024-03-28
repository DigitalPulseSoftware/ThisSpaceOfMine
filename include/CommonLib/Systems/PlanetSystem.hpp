// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_PLANETSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_PLANETSYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API PlanetSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = -1'000'000;
			using Components = Nz::TypeList<class PlanetComponent>;

			inline PlanetSystem(entt::registry& registry);
			PlanetSystem(const PlanetSystem&) = delete;
			PlanetSystem(PlanetSystem&&) = delete;
			~PlanetSystem() = default;

			void Update(Nz::Time elapsedTime);

			PlanetSystem& operator=(const PlanetSystem&) = delete;
			PlanetSystem& operator=(PlanetSystem&&) = delete;

		private:
			entt::registry& m_registry;
	};
}

#include <CommonLib/Systems/PlanetSystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_PLANETSYSTEM_HPP
