// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Systems/PlanetDatabaseSystem.hpp>
#include <CommonLib/EntityClass.hpp>
#include <ServerLib/Components/DatabaseComponent.hpp>
#include <ServerLib/Database/ServerDatabase.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

namespace tsom
{
	PlanetDatabaseSystem::PlanetDatabaseSystem(entt::registry& registry, ServerDatabase& database, Nz::UInt32 databaseId) :
	m_databaseObserver(registry),
	m_databaseDistributionObserver(registry),
	m_registry(registry),
	m_database(database),
	m_databaseId(databaseId)
	{
		m_databaseObserver.OnEntityAdded.Connect([this](entt::entity entity)
		{
			DatabaseComponent& entityDatabase = m_registry.get<DatabaseComponent>(entity);
			if (!entityDatabase.planetDatabaseId)
			{
				m_pendingUpdates.push_back({
					.operation = DatabaseOperation::Create,
					.entity = entity
				});
			}

			// Monitor changes
			EntityData& entityData = m_databaseObserver.Get(entity);

			Nz::NodeComponent& entityNode = m_registry.get<Nz::NodeComponent>(entity);
			entityData.onNodeInvalidation.Connect(entityNode.OnNodeInvalidation, [this, entity](const Nz::Node* /*node*/)
			{
				m_updatedEntities.emplace(entity);
			});

			ClassInstanceComponent& classInstance = m_registry.get<ClassInstanceComponent>(entity);
			entityData.onPropertyUpdate.Connect(classInstance.OnPropertyUpdate, [this, entity](ClassInstanceComponent* /*classInstance*/, Nz::UInt32 /*propertyIndex*/, const EntityProperty& /*newValue*/)
			{
				m_updatedEntities.emplace(entity);
			});
		});

		m_databaseDistributionObserver.OnEntityAdded.Connect([this](entt::entity entity)
		{
			EntityDistributionData& entityData = m_databaseDistributionObserver.Get(entity);

			DistributionComponent& distributionComponent = m_registry.get<DistributionComponent>(entity);
			entityData.onOutputedChanged.Connect(distributionComponent.OnOutputChanged, [this, entity](DistributionComponent*, std::size_t /*outputIndex*/)
			{
				m_updatedEntities.emplace(entity);
			});
		});

		m_databaseObserver.OnEntityRemove.Connect([this](entt::entity entity)
		{
			m_updatedEntities.remove(entity);

			DatabaseComponent& entityDatabase = m_registry.get<DatabaseComponent>(entity);
			if (entityDatabase.planetDatabaseId)
			{
				m_pendingUpdates.push_back({
					.operation = DatabaseOperation::Destroy,
					.databaseId = *entityDatabase.planetDatabaseId
				});
			}
		});
	}

	void PlanetDatabaseSystem::Save()
	{
		for (entt::entity entity : m_updatedEntities)
		{
			DatabaseComponent& entityDatabase = m_registry.get<DatabaseComponent>(entity);

			m_pendingUpdates.push_back({
				.operation = DatabaseOperation::Update,
				.entity = entity,
				.databaseId = *entityDatabase.planetDatabaseId
			});
		}

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
						planetEntity.connections = BuildConnectionJson(update.entity);

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
						partialUpdate.connections = BuildConnectionJson(update.entity);

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

	nlohmann::json PlanetDatabaseSystem::BuildConnectionJson(entt::entity entity)
	{
		DistributionComponent* distributionComponent = m_registry.try_get<DistributionComponent>(entity);
		if (!distributionComponent || distributionComponent->GetOutputCount() == 0)
			return nlohmann::json::object();

		nlohmann::json connectionsArray = nlohmann::json::array();
		for (std::size_t outputIndex = 0; outputIndex < distributionComponent->GetOutputCount(); ++outputIndex)
		{
			entt::handle connectedEntity = distributionComponent->GetOutputConnectedEntity(outputIndex);
			if (!connectedEntity)
				continue;

			DatabaseComponent* entityDatabase = connectedEntity.try_get<DatabaseComponent>();
			if (!entityDatabase)
				continue; //< Can't save connections to ephemeral entities

			connectionsArray.push_back(nlohmann::json::object({
				{"source_port", outputIndex},
				{"target_entity", entityDatabase->uniqueId.ToString()},
				{"target_port", distributionComponent->GetOutputConnectedPort(outputIndex)}
			}));
		}

		nlohmann::json connectionsDoc;
		connectionsDoc["version"] = 1;
		connectionsDoc["connections"] = connectionsArray;

		return connectionsDoc;
	}
}
