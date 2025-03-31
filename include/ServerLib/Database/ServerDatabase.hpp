// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_DATABASE_SERVERDATABASE_HPP
#define TSOM_SERVERLIB_DATABASE_SERVERDATABASE_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/Database/Schema.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <SQLiteCpp/Database.h>
#include <optional>
#include <string>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class TSOM_SERVERLIB_API ServerDatabase
	{
		public:
			ServerDatabase(Nz::ApplicationBase& app, const std::string& filename);
			ServerDatabase(const ServerDatabase&) = delete;
			ServerDatabase(ServerDatabase&&) = delete;
			~ServerDatabase() = default;

			void GetAllPlanets(Nz::FunctionRef<bool(Database::Planet&& /*planet*/)> callback) const;
			void GetAllPlanetLinks(Nz::FunctionRef<bool(Database::PlanetLink&& /*planetLink*/)> callback) const;
			void GetPlanetChunks(Nz::UInt32 planetId, Nz::FunctionRef<bool(Database::PlanetChunk&& /*PlanetChunk*/)> callback) const;

			void StorePlanet(const Database::Planet& planetChunk);
			void StorePlanetChunk(const Database::PlanetChunk& planetChunk);
			void StorePlanetLink(const Database::PlanetLink& planetChunk);

			ServerDatabase& operator=(const ServerDatabase&) = delete;
			ServerDatabase& operator=(ServerDatabase&&) = delete;

		private:
			void Migrate();
			void PrepareQueries();

			struct PreparedStatements
			{
				PreparedStatements(SQLite::Database& database);

				SQLite::Statement getAllPlanetQuery;
				SQLite::Statement getAllPlanetLinkQuery;
				SQLite::Statement getPlanetChunkQuery;
				SQLite::Statement storePlanetQuery;
				SQLite::Statement storePlanetChunkQuery;
				SQLite::Statement storePlanetLinkQuery;
			};

			SQLite::Database m_database;
			mutable std::optional<PreparedStatements> m_preparedStatements;
			Nz::ApplicationBase& m_app;
	};
}


#endif // TSOM_SERVERLIB_DATABASE_SERVERDATABASE_HPP
