// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Error.hpp>

namespace tsom
{
	template<typename F>
	void ServerEnvironment::ForEachConnectedEnvironment(F&& callback) const
	{
		for (auto&& [environment, transform] : m_connectedEnvironments)
			callback(*environment, transform);
	}

	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback)
	{
		for (std::size_t playerIndex : m_registeredPlayers.IterBits())
			callback(*m_serverInstance.GetPlayer(playerIndex));
	}

	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback) const
	{
		for (std::size_t playerIndex : m_registeredPlayers.IterBits())
			callback(*m_serverInstance.GetPlayer(playerIndex));
	}

	inline bool ServerEnvironment::GetEnvironmentTransformation(ServerEnvironment& targetEnv, EnvironmentTransform* transform) const
	{
		auto it = m_connectedEnvironments.find(&targetEnv);
		if (it == m_connectedEnvironments.end())
			return false;

		*transform = it->second;
		return true;
	}

	inline Nz::EnttWorld& ServerEnvironment::GetWorld()
	{
		return m_world;
	}

	inline const Nz::EnttWorld& ServerEnvironment::GetWorld() const
	{
		return m_world;
	}
}
