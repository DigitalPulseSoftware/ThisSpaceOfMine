// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>

namespace tsom
{
	template<typename F>
	void ClientSessionHandler::ForEachPlayer(F&& callback) const
	{
		for (PlayerIndex index = 0; index < m_players.size(); ++index)
		{
			auto& playerInfoOpt = m_players[index];
			if (playerInfoOpt)
				callback(index, *playerInfoOpt);
		}
	}

	inline entt::handle ClientSessionHandler::GetControlledEntity() const
	{
		return m_playerControlledEntity;
	}

	inline const GravityController* ClientSessionHandler::GetGravityController(std::size_t environmentIndex) const
	{
		if (environmentIndex > m_environments.size() || !m_environments[environmentIndex])
			return nullptr;

		return m_environments[environmentIndex]->gravityController;
	}

	inline ScriptingContext& ClientSessionHandler::GetScriptingContext()
	{
		return m_scriptingContext;
	}

	inline auto ClientSessionHandler::FetchPlayerInfo(PlayerIndex playerIndex) -> PlayerInfo*
	{
		if (playerIndex >= m_players.size() || !m_players[playerIndex])
			return nullptr;

		return &m_players[playerIndex].value();
	}

	inline auto ClientSessionHandler::FetchPlayerInfo(PlayerIndex playerIndex) const -> const PlayerInfo*
	{
		if (playerIndex >= m_players.size() || !m_players[playerIndex])
			return nullptr;

		return &m_players[playerIndex].value();
	}
}
