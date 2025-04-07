// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Database/ServerDatabase.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <NazaraUtils/CallOnExit.hpp>
#include <SQLiteCpp/Transaction.h>
#include <fmt/color.h>
#include <tsl/hopscotch_set.h>

namespace tsom
{
	ServerDatabase::PreparedStatements::PreparedStatements(SQLite::Database& database) :
	createPlanetEntityQuery(database, "INSERT INTO planet_entities(unique_id, planet_id, class_name, class_version, position_x, position_y, position_z, rotation_x, rotation_y, rotation_z, rotation_w, properties, last_update) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime()) RETURNING id"),
	deletePlanetEntityQuery(database, "DELETE FROM planet_entities WHERE id = ?"),
	getAllPlanetEntitiesQuery(database, "SELECT id, unique_id, class_name, class_version, position_x, position_y, position_z, rotation_x, rotation_y, rotation_z, rotation_w, properties FROM planet_entities WHERE planet_id = ?"),
	partialUpdatePlanetEntityQuery(database, "UPDATE planet_entities SET class_version = ?, position_x = ?, position_y = ?, position_z = ?, rotation_x = ?, rotation_y = ?, rotation_z = ?, rotation_w = ?, properties = ?, last_update = datetime() WHERE id = ?"),
	getAllPlanetQuery(database, "SELECT id, generator, seed, chunk_count_x, chunk_count_y, chunk_count_z, corner_radius, gravity FROM planets"),
	getAllPlanetLinkQuery(database, "SELECT source_planet_id, destination_planet_id, position_x, position_y, position_z FROM planet_links"),
	getPlanetChunkQuery(database, "SELECT position_x, position_y, position_z, version, chunk_data FROM planet_chunks WHERE planet_id = ?"),
	storePlanetQuery(database, "REPLACE INTO planets(id, generator, seed, chunk_count_x, chunk_count_y, chunk_count_z, corner_radius, gravity) VALUES(?, ?, ?, ?, ?, ?, ?, ?)"),
	storePlanetChunkQuery(database, "REPLACE INTO planet_chunks(planet_id, position_x, position_y, position_z, version, chunk_data, last_update) VALUES(?, ?, ?, ?, ?, ?, datetime())"),
	storePlanetLinkQuery(database, "REPLACE INTO planet_links(source_planet_id, destination_planet_id, position_x, position_y, position_z) VALUES(?, ?, ?, ?, ?)")
	{
	}

	ServerDatabase::ServerDatabase(Nz::ApplicationBase& app, const std::string& filename) :
	m_database(filename, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE),
	m_app(app)
	{
		Migrate();
		PrepareQueries();
	}

	Nz::UInt32 ServerDatabase::CreatePlanetEntity(const Database::PlanetEntity& planetEntity)
	{
		NAZARA_DEFER({ m_preparedStatements->createPlanetEntityQuery.reset(); });

		m_preparedStatements->createPlanetEntityQuery.bind(1, planetEntity.uniqueId.ToString());
		m_preparedStatements->createPlanetEntityQuery.bind(2, planetEntity.planetId);
		m_preparedStatements->createPlanetEntityQuery.bindNoCopy(3, planetEntity.className.data());
		m_preparedStatements->createPlanetEntityQuery.bind(4, planetEntity.classVersion);
		m_preparedStatements->createPlanetEntityQuery.bind(5, planetEntity.position.x);
		m_preparedStatements->createPlanetEntityQuery.bind(6, planetEntity.position.y);
		m_preparedStatements->createPlanetEntityQuery.bind(7, planetEntity.position.z);
		m_preparedStatements->createPlanetEntityQuery.bind(8, planetEntity.rotation.x);
		m_preparedStatements->createPlanetEntityQuery.bind(9, planetEntity.rotation.y);
		m_preparedStatements->createPlanetEntityQuery.bind(10, planetEntity.rotation.z);
		m_preparedStatements->createPlanetEntityQuery.bind(11, planetEntity.rotation.w);
		m_preparedStatements->createPlanetEntityQuery.bind(12, planetEntity.properties.dump());

		m_preparedStatements->createPlanetEntityQuery.executeStep();

		return m_preparedStatements->createPlanetEntityQuery.getColumn(0);
	}

	void ServerDatabase::DeletePlanetEntity(Nz::UInt32 planetEntityId)
	{
		NAZARA_DEFER({ m_preparedStatements->deletePlanetEntityQuery.reset(); });

		m_preparedStatements->deletePlanetEntityQuery.bind(1, planetEntityId);

		m_preparedStatements->deletePlanetEntityQuery.exec();
	}

	void ServerDatabase::GetAllPlanets(Nz::FunctionRef<bool(Database::Planet&& /*planet*/)> callback) const
	{
		NAZARA_DEFER({ m_preparedStatements->getAllPlanetQuery.reset(); });
		while (m_preparedStatements->getAllPlanetQuery.executeStep())
		{
			Database::Planet planet;
			planet.id = m_preparedStatements->getAllPlanetQuery.getColumn(0);
			planet.generatorName = m_preparedStatements->getAllPlanetQuery.getColumn(1).getText();
			planet.seed = m_preparedStatements->getAllPlanetQuery.getColumn(2);
			planet.chunkCount.x = m_preparedStatements->getAllPlanetQuery.getColumn(3);
			planet.chunkCount.y = m_preparedStatements->getAllPlanetQuery.getColumn(4);
			planet.chunkCount.z = m_preparedStatements->getAllPlanetQuery.getColumn(5);
			planet.cornerRadius = Nz::SafeCaster(m_preparedStatements->getAllPlanetQuery.getColumn(6).getDouble());
			planet.gravity = Nz::SafeCaster(m_preparedStatements->getAllPlanetQuery.getColumn(7).getDouble());

			if (!callback(std::move(planet)))
				break;
		}
	}

	void ServerDatabase::GetAllPlanetEntities(Nz::UInt32 planetEntityId, Nz::FunctionRef<bool(Database::PlanetEntity&& /*planetEntities*/)> callback) const
	{
		NAZARA_DEFER({ m_preparedStatements->getAllPlanetEntitiesQuery.reset(); });

		m_preparedStatements->getAllPlanetEntitiesQuery.bind(1, planetEntityId);

		while (m_preparedStatements->getAllPlanetEntitiesQuery.executeStep())
		{
			Database::PlanetEntity planetEntity;
			planetEntity.planetId = planetEntityId;

			planetEntity.id = m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(0);
			planetEntity.uniqueId = Nz::Uuid::FromString(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(1).getText());
			planetEntity.className = m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(2).getText();
			planetEntity.classVersion = m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(3);
			planetEntity.position.x = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(4).getDouble());
			planetEntity.position.y = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(5).getDouble());
			planetEntity.position.z = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(6).getDouble());
			planetEntity.rotation.x = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(7).getDouble());
			planetEntity.rotation.y = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(8).getDouble());
			planetEntity.rotation.z = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(9).getDouble());
			planetEntity.rotation.w = Nz::SafeCaster(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(10).getDouble());
			planetEntity.properties = nlohmann::json::parse(m_preparedStatements->getAllPlanetEntitiesQuery.getColumn(11).getText());

			if (!callback(std::move(planetEntity)))
				break;
		}
	}

	void ServerDatabase::GetAllPlanetLinks(Nz::FunctionRef<bool(Database::PlanetLink&& /*planetLink*/)> callback) const
	{
		NAZARA_DEFER({ m_preparedStatements->getAllPlanetLinkQuery.reset(); });
		while (m_preparedStatements->getAllPlanetLinkQuery.executeStep())
		{
			Database::PlanetLink planetLink;
			planetLink.sourcePlanet = m_preparedStatements->getAllPlanetLinkQuery.getColumn(0);
			planetLink.destinationPlanet = m_preparedStatements->getAllPlanetLinkQuery.getColumn(1);
			planetLink.position.x = Nz::SafeCaster(m_preparedStatements->getAllPlanetLinkQuery.getColumn(2).getDouble());
			planetLink.position.y = Nz::SafeCaster(m_preparedStatements->getAllPlanetLinkQuery.getColumn(3).getDouble());
			planetLink.position.z = Nz::SafeCaster(m_preparedStatements->getAllPlanetLinkQuery.getColumn(4).getDouble());

			if (!callback(std::move(planetLink)))
				break;
		}
	}

	void ServerDatabase::GetPlanetChunks(Nz::UInt32 planetId, Nz::FunctionRef<bool(Database::PlanetChunk&& /*planetChunk*/)> callback) const
	{
		NAZARA_DEFER({ m_preparedStatements->getPlanetChunkQuery.reset(); });
		m_preparedStatements->getPlanetChunkQuery.bind(1, planetId);

		while (m_preparedStatements->getPlanetChunkQuery.executeStep())
		{
			Database::PlanetChunk planetChunk;
			planetChunk.planetId = planetId;
			planetChunk.position.x = Nz::SafeCaster(m_preparedStatements->getPlanetChunkQuery.getColumn(0).getDouble());
			planetChunk.position.y = Nz::SafeCaster(m_preparedStatements->getPlanetChunkQuery.getColumn(1).getDouble());
			planetChunk.position.z = Nz::SafeCaster(m_preparedStatements->getPlanetChunkQuery.getColumn(2).getDouble());
			planetChunk.version = m_preparedStatements->getPlanetChunkQuery.getColumn(3);

			SQLite::Column column = m_preparedStatements->getPlanetChunkQuery.getColumn(4);
			const Nz::UInt8* chunkData = static_cast<const Nz::UInt8*>(column.getBlob());
			planetChunk.chunkData = std::span(chunkData, chunkData + column.getBytes());

			if (!callback(std::move(planetChunk)))
				break;
		}
	}

	void ServerDatabase::Migrate()
	{
		tsl::hopscotch_set<std::string, std::hash<std::string_view>, std::equal_to<>> migrated;
		if (m_database.tableExists("migration"))
		{
			SQLite::Statement query(m_database, "SELECT name FROM migration");
			while (query.executeStep())
				migrated.insert(query.getColumn(0));
		}

		std::vector<std::string> remainingMigrationScripts;

		auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();
		fs.IterateOnDirectory("database/server", [&](std::string_view entryName, Nz::VirtualDirectory::Entry entry)
		{
			if (!std::holds_alternative<Nz::VirtualDirectory::FileEntry>(entry))
				return; //< not a file

			if (!Nz::EndsWith(entryName, ".sql"))
				return; //< not a migration script

			if (!migrated.contains(entryName))
				remainingMigrationScripts.emplace_back(entryName);
		});

		if (remainingMigrationScripts.empty())
			return;

		std::sort(remainingMigrationScripts.begin(), remainingMigrationScripts.end());

		SQLite::Transaction transaction(m_database);

		for (const std::string& migrationScript : remainingMigrationScripts)
		{
			try
			{
				bool success = fs.GetFileContent(fmt::format("database/server/{0}", migrationScript), [&](const void* data, Nz::UInt64 size)
				{
					std::string scriptContent(static_cast<const char*>(data), Nz::SafeCast<std::size_t>(size));
					m_database.exec(scriptContent);
				});

				// We have to create the statement after the first migration script has been executed
				SQLite::Statement insertTransaction(m_database, "INSERT INTO migration(name) VALUES(?)");
				insertTransaction.bindNoCopy(1, migrationScript.c_str());
				insertTransaction.exec();

				if (!success)
					throw std::runtime_error("failed to read");
			}
			catch (const std::exception& e)
			{
				fmt::print(fg(fmt::color::red), "[Database Migration] failed to execute migration script {0}: {1}", migrationScript, e.what());
				transaction.rollback();
			}
		}

		transaction.commit();
	}

	void ServerDatabase::PrepareQueries()
	{
		m_preparedStatements.emplace(m_database);
	}

	void ServerDatabase::StorePlanet(const Database::Planet& planet)
	{
		NAZARA_DEFER({ m_preparedStatements->storePlanetQuery.reset(); });

		m_preparedStatements->storePlanetQuery.bind(1, planet.id);
		m_preparedStatements->storePlanetQuery.bindNoCopy(2, planet.generatorName.data());
		m_preparedStatements->storePlanetQuery.bind(3, planet.seed);
		m_preparedStatements->storePlanetQuery.bind(4, planet.chunkCount.x);
		m_preparedStatements->storePlanetQuery.bind(5, planet.chunkCount.y);
		m_preparedStatements->storePlanetQuery.bind(6, planet.chunkCount.z);
		m_preparedStatements->storePlanetQuery.bind(7, planet.cornerRadius);
		m_preparedStatements->storePlanetQuery.bind(8, planet.gravity);

		m_preparedStatements->storePlanetQuery.exec();
	}

	void ServerDatabase::StorePlanetChunk(const Database::PlanetChunk& planetChunk)
	{
		NAZARA_DEFER({ m_preparedStatements->storePlanetChunkQuery.reset(); });

		m_preparedStatements->storePlanetChunkQuery.bind(1, planetChunk.planetId);
		m_preparedStatements->storePlanetChunkQuery.bind(2, planetChunk.position.x);
		m_preparedStatements->storePlanetChunkQuery.bind(3, planetChunk.position.y);
		m_preparedStatements->storePlanetChunkQuery.bind(4, planetChunk.position.z);
		m_preparedStatements->storePlanetChunkQuery.bind(5, planetChunk.version);
		m_preparedStatements->storePlanetChunkQuery.bindNoCopy(6, planetChunk.chunkData.data(), Nz::SafeCaster(planetChunk.chunkData.size()));

		m_preparedStatements->storePlanetChunkQuery.exec();
	}

	void ServerDatabase::StorePlanetLink(const Database::PlanetLink& planetLink)
	{
		NAZARA_DEFER({ m_preparedStatements->storePlanetLinkQuery.reset(); });

		m_preparedStatements->storePlanetLinkQuery.bind(1, planetLink.sourcePlanet);
		m_preparedStatements->storePlanetLinkQuery.bind(2, planetLink.destinationPlanet);
		m_preparedStatements->storePlanetLinkQuery.bind(3, planetLink.position.x);
		m_preparedStatements->storePlanetLinkQuery.bind(4, planetLink.position.y);
		m_preparedStatements->storePlanetLinkQuery.bind(5, planetLink.position.z);

		m_preparedStatements->storePlanetLinkQuery.exec();
	}

	void ServerDatabase::Transaction(Nz::FunctionRef<bool(ServerDatabase& database)> callback)
	{
		SQLite::Transaction transaction(m_database);
		if (callback(*this))
			transaction.commit();
		else
			transaction.rollback();
	}

	void ServerDatabase::UpdatePlanetEntity(Nz::UInt32 planetEntityId, const Database::PlanetEntityPartial& planetEntity)
	{
		NAZARA_DEFER({ m_preparedStatements->partialUpdatePlanetEntityQuery.reset(); });

		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(1, planetEntity.classVersion);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(2, planetEntity.position.x);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(3, planetEntity.position.y);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(4, planetEntity.position.z);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(5, planetEntity.rotation.x);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(6, planetEntity.rotation.y);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(7, planetEntity.rotation.z);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(8, planetEntity.rotation.w);
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(9, planetEntity.properties.dump());
		m_preparedStatements->partialUpdatePlanetEntityQuery.bind(10, planetEntityId);

		m_preparedStatements->partialUpdatePlanetEntityQuery.exec();
	}
}
