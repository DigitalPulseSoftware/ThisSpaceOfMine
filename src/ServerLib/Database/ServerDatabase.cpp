// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Database/ServerDatabase.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <fmt/color.h>
#include <tsl/hopscotch_set.h>
#include <SQLiteCpp/Transaction.h>

namespace tsom
{
	ServerDatabase::ServerDatabase(Nz::ApplicationBase& app, const std::string& filename) :
	m_database(filename, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE),
	m_app(app)
	{
	}

	std::vector<Database::Planet> ServerDatabase::GetAllPlanets() const
	{
		std::vector<Database::Planet> planets;

		SQLite::Statement query(m_database, "SELECT id, generator, seed, chunk_count_x, chunk_count_y, chunk_count_z, corner_radius, gravity FROM planets");
		while (query.executeStep())
		{
			auto& planet = planets.emplace_back();
			planet.id = query.getColumn(0);
			planet.generatorName = query.getColumn(1).getString();
			planet.seed = query.getColumn(2);
			planet.chunkCount.x = query.getColumn(3);
			planet.chunkCount.y = query.getColumn(4);
			planet.chunkCount.z = query.getColumn(5);
			planet.cornerRadius = Nz::SafeCaster(query.getColumn(6).getDouble());
			planet.gravity = Nz::SafeCaster(query.getColumn(7).getDouble());
		}

		return planets;
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
}
