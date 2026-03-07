// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ServerPlayerList::ServerPlayerList(ServerInstance& serverInstance) :
	m_serverInstance(&serverInstance)
	{
	}

	template<typename F> void ServerPlayerList::ForEachPlayer(F&& callback)
	{
		for (std::size_t playerIndex : m_registeredPlayers.IterBits())
			callback(*m_serverInstance->GetPlayer(playerIndex));
	}

	template<typename F> void ServerPlayerList::ForEachPlayer(F&& callback) const
	{
		for (std::size_t playerIndex : m_registeredPlayers.IterBits())
			callback(*static_cast<const ServerInstance*>(m_serverInstance)->GetPlayer(playerIndex));
	}
}
