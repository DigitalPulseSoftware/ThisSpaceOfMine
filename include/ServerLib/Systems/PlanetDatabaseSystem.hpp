// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_PLANETDATABASESYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_PLANETDATABASESYSTEM_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Core/EnttObserver.hpp>
#include <Nazara/Core/Time.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>
#include <nlohmann/json_fwd.hpp>
#include <tsl/hopscotch_map.h>
#include <vector>

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
			~PlanetDatabaseSystem() = default;

			void Save();

			PlanetDatabaseSystem& operator=(const PlanetDatabaseSystem&) = delete;
			PlanetDatabaseSystem& operator=(PlanetDatabaseSystem&&) = delete;

		private:
			nlohmann::json BuildConnectionJson(entt::entity entity);

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

			struct EntityData
			{
				NazaraSlot(Nz::NodeComponent, OnNodeInvalidation, onNodeInvalidation);
				NazaraSlot(ClassInstanceComponent, OnPropertyUpdate, onPropertyUpdate);
			};

			struct EntityDistributionData
			{
				NazaraSlot(DistributionComponent, OnOutputChanged, onOutputedChanged);
			};

			using ComponentList = Nz::TypeList<Nz::NodeComponent, class DatabaseComponent, ClassInstanceComponent>;
			using ExcludedComponents = Nz::TypeList<>;

			Nz::EnttObserver<ComponentList, ExcludedComponents, EntityData> m_databaseObserver;
			Nz::EnttObserver<Nz::TypeListAppend<ComponentList, DistributionComponent>, ExcludedComponents, EntityDistributionData> m_databaseDistributionObserver;
			entt::storage<void> m_updatedEntities;
			std::vector<DatabaseUpdate> m_pendingUpdates;
			entt::registry& m_registry;
			ServerDatabase& m_database;
			Nz::UInt32 m_databaseId;
	};
}

#include <ServerLib/Systems/PlanetDatabaseSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_PLANETDATABASESYSTEM_HPP
