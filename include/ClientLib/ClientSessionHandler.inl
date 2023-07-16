// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>

namespace tsom
{
	inline entt::handle ClientSessionHandler::GetControlledEntity() const
	{
		return m_playerControlledEntity;
	}

	inline auto ClientSessionHandler::FetchPlayerInfo(PlayerIndex playerIndex) const -> const PlayerInfo*
	{
		if (playerIndex >= m_players.size() || !m_players[playerIndex])
			return nullptr;

		return &m_players[playerIndex].value();
	}
}
