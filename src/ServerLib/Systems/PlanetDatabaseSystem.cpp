// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/PlanetDatabaseSystem.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <ServerLib/Components/DatabaseComponent.hpp>
#include <ServerLib/Database/ServerDatabase.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

namespace tsom
{
	PlanetDatabaseSystem::PlanetDatabaseSystem(entt::registry& registry, ServerDatabase& database, Nz::UInt32 databaseId) :
	m_databaseObserver(registry),
	m_registry(registry),
	m_database(database),
	m_databaseId(databaseId)
	{
		m_databaseObserver.OnEntityAdded.Connect([&](entt::entity entity)
		{
			DatabaseComponent& entityDatabase = m_registry.get<DatabaseComponent>(entity);
			if (entityDatabase.planetDatabaseId)
			{
				// Entity was unknown, be conservative and update it
				m_pendingUpdates.push_back({
					.operation = DatabaseOperation::Update,
					.entity = entity,
					.databaseId = *entityDatabase.planetDatabaseId
				});
			}
			else
			{
				m_pendingUpdates.push_back({
					.operation = DatabaseOperation::Create,
					.entity = entity
				});
			}
		});

		m_databaseObserver.OnEntityRemove.Connect([&](entt::entity entity)
		{
			DatabaseComponent& entityDatabase = m_registry.get<DatabaseComponent>(entity);
			if (entityDatabase.planetDatabaseId)
			{
				m_pendingUpdates.push_back({
					.operation = DatabaseOperation::Destroy,
					.databaseId = databaseId
				});
			}
		});
	}

	void PlanetDatabaseSystem::Save()
	{
		if (m_pendingUpdates.empty())
			return;

		m_database.Transaction([&](ServerDatabase& database)
		{
			bool commitTransaction = false;

			for (DatabaseUpdate& update : m_pendingUpdates)
			{
				if (update.operation != DatabaseOperation::Destroy && !m_registry.valid(update.entity))
					continue;

				switch (update.operation)
				{
					case DatabaseOperation::Create:
					{
						ClassInstanceComponent& classInstance = m_registry.get<ClassInstanceComponent>(update.entity);
						DatabaseComponent& entityDatabase = m_registry.get<DatabaseComponent>(update.entity);
						Nz::NodeComponent& entityNode = m_registry.get<Nz::NodeComponent>(update.entity);

						const auto& entityClass = classInstance.GetClass();

						// Create
						Database::PlanetEntity planetEntity;
						planetEntity.planetId = m_databaseId;
						planetEntity.uniqueId = entityDatabase.uniqueId;
						planetEntity.className = entityClass->GetName();
						planetEntity.classVersion = 1; //< TODO
						planetEntity.position = entityNode.GetPosition();
						planetEntity.rotation = entityNode.GetRotation();
						planetEntity.properties = entityClass->PropertiesToJson(classInstance.GetProperties());

						entityDatabase.planetDatabaseId = m_database.CreatePlanetEntity(planetEntity);
						break;
					}

					case DatabaseOperation::Destroy:
					{
						m_database.DeletePlanetEntity(update.databaseId);
						break;
					}

					case DatabaseOperation::Update:
					{
						ClassInstanceComponent& classInstance = m_registry.get<ClassInstanceComponent>(update.entity);
						Nz::NodeComponent& entityNode = m_registry.get<Nz::NodeComponent>(update.entity);

						const auto& entityClass = classInstance.GetClass();

						// Update
						Database::PlanetEntityPartial partialUpdate;
						partialUpdate.classVersion = 1; //< TODO
						partialUpdate.position = entityNode.GetPosition();
						partialUpdate.rotation = entityNode.GetRotation();
						partialUpdate.properties = entityClass->PropertiesToJson(classInstance.GetProperties());

						m_database.UpdatePlanetEntity(update.databaseId, partialUpdate);
						break;
					}
				}

				commitTransaction = true;
			}
			m_pendingUpdates.clear();

			return commitTransaction;
		});
	}
}
