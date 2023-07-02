// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/NetworkSessionManager.hpp>

namespace tsom
{
	NetworkSession::NetworkSession(NetworkReactor& reactor, std::size_t peerId, const Nz::IpAddress& remoteAddress) :
	m_peerId(peerId),
	m_reactor(reactor),
	m_remoteAddress(remoteAddress)
	{
	}

	NetworkSession::~NetworkSession() = default;

	void NetworkSession::Disconnect()
	{
		assert(m_peerId != NetworkReactor::InvalidPeerId);

		m_reactor.DisconnectPeer(m_peerId);
	}

	void NetworkSession::HandlePacket(Nz::NetPacket&& netPacket)
	{
		m_sessionHandler->HandlePacket(std::move(netPacket));
	}

	void NetworkSession::SetHandler(std::unique_ptr<SessionHandler>&& sessionHandler)
	{
		m_sessionHandler = std::move(sessionHandler);
	}
}
