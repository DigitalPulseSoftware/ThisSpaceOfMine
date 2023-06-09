#include "ServerPlayer.hpp"
// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline ServerPlayer::ServerPlayer(ServerInstance& instance, std::size_t playerIndex, NetworkSession* session, std::string nickname) :
	m_playerIndex(playerIndex),
	m_nickname(std::move(nickname)),
	m_session(session),
	m_visibilityHandler(m_session),
	m_instance(instance)
	{
	}

	inline const std::string& ServerPlayer::GetNickname() const
	{
		return m_nickname;
	}

	inline ServerInstance& ServerPlayer::GetServerInstance()
	{
		return m_instance;
	}

	inline const ServerInstance& ServerPlayer::GetServerInstance() const
	{
		return m_instance;
	}

	inline SessionVisibilityHandler& ServerPlayer::GetVisibilityHandler()
	{
		return m_visibilityHandler;
	}

	inline const SessionVisibilityHandler& ServerPlayer::GetVisibilityHandler() const
	{
		return m_visibilityHandler;
	}
}
