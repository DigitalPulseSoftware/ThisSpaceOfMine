// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_TEMPSHIPENTRYSYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_TEMPSHIPENTRYSYSTEM_HPP

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

	class TSOM_SERVERLIB_API TempShipEntrySystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = -1; //< execute before physics
			using Components = Nz::TypeList<Nz::NodeComponent, class TempShipEntryComponent>;

			inline TempShipEntrySystem(entt::registry& registry, ServerEnvironment* ownerEnvironment);
			TempShipEntrySystem(const TempShipEntrySystem&) = delete;
			TempShipEntrySystem(TempShipEntrySystem&&) = delete;
			~TempShipEntrySystem() = default;

			void Update(Nz::Time elapsedTime);

			TempShipEntrySystem& operator=(const TempShipEntrySystem&) = delete;
			TempShipEntrySystem& operator=(TempShipEntrySystem&&) = delete;

		private:
			entt::registry& m_registry;
			ServerEnvironment* m_ownerEnvironment;
	};
}

#include <ServerLib/Systems/TempShipEntrySystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_TEMPSHIPENTRYSYSTEM_HPP
