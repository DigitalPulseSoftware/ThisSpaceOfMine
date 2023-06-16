// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/PlayerSessionHandler.hpp>

namespace tsom
{
	inline NetworkSession::NetworkSession(NetworkSessionManager& owner, std::size_t peerId, const Nz::IpAddress& remoteAddres) :
	m_peerId(peerId),
	m_owner(owner),
	m_remoteAddress(remoteAddres)
	{
		m_sessionHandler = std::make_unique<PlayerSessionHandler>();
	}

	inline void tsom::NetworkSession::HandlePacket(Nz::NetPacket&& netPacket)
	{
		m_sessionHandler->HandlePacket(std::move(netPacket));
	}

	inline void NetworkSession::SetHandler(std::unique_ptr<SessionHandler>&& sessionHandler)
	{
		m_sessionHandler = std::move(sessionHandler);
	}
}
