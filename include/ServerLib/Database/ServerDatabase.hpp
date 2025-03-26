// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_DATABASE_SERVERDATABASE_HPP
#define TSOM_SERVERLIB_DATABASE_SERVERDATABASE_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/Database/Schema.hpp>
#include <SQLiteCpp/Database.h>
#include <string>
#include <vector>

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

			std::vector<Database::Planet> GetAllPlanets() const;

			void Migrate();

			ServerDatabase& operator=(const ServerDatabase&) = delete;
			ServerDatabase& operator=(ServerDatabase&&) = delete;

		private:
			SQLite::Database m_database;
			Nz::ApplicationBase& m_app;
	};
}


#endif // TSOM_SERVERLIB_DATABASE_SERVERDATABASE_HPP
