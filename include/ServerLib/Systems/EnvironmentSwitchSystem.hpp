// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_ENVIRONMENTSWITCHSYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_ENVIRONMENTSWITCHSYSTEM_HPP

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

	class TSOM_SERVERLIB_API EnvironmentSwitchSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = -1; //< execute before physics
			using Components = Nz::TypeList<Nz::NodeComponent, class EnvironmentEnterTriggerComponent>;

			inline EnvironmentSwitchSystem(entt::registry& registry, ServerEnvironment* ownerEnvironment);
			EnvironmentSwitchSystem(const EnvironmentSwitchSystem&) = delete;
			EnvironmentSwitchSystem(EnvironmentSwitchSystem&&) = delete;
			~EnvironmentSwitchSystem() = default;

			void Update(Nz::Time elapsedTime);

			EnvironmentSwitchSystem& operator=(const EnvironmentSwitchSystem&) = delete;
			EnvironmentSwitchSystem& operator=(EnvironmentSwitchSystem&&) = delete;

		private:
			entt::registry& m_registry;
			ServerEnvironment* m_ownerEnvironment;
	};
}

#include <ServerLib/Systems/EnvironmentSwitchSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_ENVIRONMENTSWITCHSYSTEM_HPP
