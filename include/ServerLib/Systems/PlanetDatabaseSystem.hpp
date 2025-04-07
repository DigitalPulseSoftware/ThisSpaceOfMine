// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_ENTITYDATABASESYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_ENTITYDATABASESYSTEM_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <vector>

namespace Nz
{
	class NodeComponent;
}

namespace tsom
{
	class ServerDatabase;

	class TSOM_SERVERLIB_API PlanetDatabaseSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = 1'000'0000; //< execute after everything
			using Components = Nz::TypeList<Nz::NodeComponent, class DatabaseComponent, class ClassInstanceComponent>;

			PlanetDatabaseSystem(entt::registry& registry, ServerDatabase& database, Nz::UInt32 databaseId);
			PlanetDatabaseSystem(const PlanetDatabaseSystem&) = delete;
			PlanetDatabaseSystem(PlanetDatabaseSystem&&) = delete;
			~PlanetDatabaseSystem();

			void Save();

			PlanetDatabaseSystem& operator=(const PlanetDatabaseSystem&) = delete;
			PlanetDatabaseSystem& operator=(PlanetDatabaseSystem&&) = delete;

		private:
			enum class DatabaseOperation
			{
				Create,
				Destroy,
				Update
			};

			struct DatabaseUpdate
			{
				DatabaseOperation operation;
				entt::entity entity; // Only for Create/Update
				Nz::UInt32 databaseId; // Only for Update/Destroy
			};

			entt::observer m_databaseObserver;
			std::vector<DatabaseUpdate> m_pendingUpdates;
			tsl::hopscotch_map<entt::entity, Nz::UInt32> m_trackedEntities;
			entt::registry& m_registry;
			ServerDatabase& m_database;
			Nz::UInt32 m_databaseId;
	};
}

#include <ServerLib/Systems/PlanetDatabaseSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_ENTITYDATABASESYSTEM_HPP
