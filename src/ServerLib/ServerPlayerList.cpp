// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlayerList.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <NazaraUtils/Assert.hpp>

namespace tsom
{
	bool ServerPlayerList::IsPlayerRegistered(ServerPlayer* player) const
	{
		return m_registeredPlayers.UnboundedTest(player->GetPlayerIndex());
	}

	void ServerPlayerList::RegisterPlayer(ServerPlayer* player)
	{
		NazaraAssertMsg(!IsPlayerRegistered(player), "player was already registered");
		m_registeredPlayers.UnboundedSet(player->GetPlayerIndex());
	}

	void ServerPlayerList::UnregisterPlayer(ServerPlayer* player)
	{
		NazaraAssertMsg(IsPlayerRegistered(player), "player is not registered");
		m_registeredPlayers.Reset(player->GetPlayerIndex());
	}
}
