// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_ATMOSPHERESYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_ATMOSPHERESYSTEM_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace Nz
{
	class NodeComponent;
}

namespace tsom
{
	class ServerEnvironment;

	class TSOM_SERVERLIB_API AtmosphereSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = -1; //< execute before physics
			using Components = Nz::TypeList<Nz::NodeComponent, class AtmosphereCarrier, class AtmosphereExchanger, class AtmosphereMonitor>;

			AtmosphereSystem(entt::registry& registry);
			AtmosphereSystem(const AtmosphereSystem&) = delete;
			AtmosphereSystem(AtmosphereSystem&&) = delete;
			~AtmosphereSystem() = default;

			void Update(Nz::Time elapsedTime);

			AtmosphereSystem& operator=(const AtmosphereSystem&) = delete;
			AtmosphereSystem& operator=(AtmosphereSystem&&) = delete;

		private:
			void PerformExchange(Nz::Time elapsedTime);
			void UpdateAtmosphere();

			entt::registry& m_registry;
			ServerEnvironment* m_ownerEnvironment;
	};
}

#include <ServerLib/Systems/AtmosphereSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_ATMOSPHERESYSTEM_HPP
