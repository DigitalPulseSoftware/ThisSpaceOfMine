// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Error.hpp>

namespace tsom
{
	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback) const
	{
		for (ServerPlayer* player : m_players)
			callback(*player);
	}

	inline Nz::EnttWorld& ServerEnvironment::GetWorld()
	{
		return m_world;
	}

	inline const Nz::EnttWorld& ServerEnvironment::GetWorld() const
	{
		return m_world;
	}

	inline void ServerEnvironment::RegisterPlayer(ServerPlayer* player)
	{
		NazaraAssert(std::find(m_players.begin(), m_players.end(), player) == m_players.end(), "player was already registered");
		m_players.push_back(player);
	}

	inline void ServerEnvironment::UnregisterPlayer(ServerPlayer* player)
	{
		auto it = std::find(m_players.begin(), m_players.end(), player);
		NazaraAssert(it != m_players.end(), "player is not registered registered");
		m_players.erase(it);
	}
}
