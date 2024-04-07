// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_ENVIRONMENTPROXYSYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_ENVIRONMENTPROXYSYSTEM_HPP

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
	class TSOM_SERVERLIB_API EnvironmentProxySystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = 1'000'000;
			using Components = Nz::TypeList<class EnvironmentProxyComponent, Nz::NodeComponent>;

			inline EnvironmentProxySystem(entt::registry& registry);
			EnvironmentProxySystem(const EnvironmentProxySystem&) = delete;
			EnvironmentProxySystem(EnvironmentProxySystem&&) = delete;
			~EnvironmentProxySystem() = default;

			void Update(Nz::Time elapsedTime);

			EnvironmentProxySystem& operator=(const EnvironmentProxySystem&) = delete;
			EnvironmentProxySystem& operator=(EnvironmentProxySystem&&) = delete;

		private:
			entt::registry& m_registry;
	};
}

#include <ServerLib/Systems/EnvironmentProxySystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_ENVIRONMENTPROXYSYSTEM_HPP
