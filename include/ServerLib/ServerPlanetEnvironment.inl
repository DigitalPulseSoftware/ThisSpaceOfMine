// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline std::optional<Nz::UInt32> ServerPlanetEnvironment::GetDatabaseId() const
	{
		return m_databaseId;
	}

	inline entt::handle ServerPlanetEnvironment::GetPlanetEntity() const
	{
		return m_planetEntity;
	}
}
