// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Assert.hpp>

namespace tsom
{
	inline void NetworkedEntitiesSystem::RegisterPlayer(ServerPlayer* player, bool createEntities)
	{
		if (createEntities)
		{
			NazaraAssert(std::find(m_pendingPlayers.begin(), m_pendingPlayers.end(), player) == m_pendingPlayers.end());
			m_pendingPlayers.push_back(player);
		}
		else
			m_players.RegisterPlayer(player);
	}
}
