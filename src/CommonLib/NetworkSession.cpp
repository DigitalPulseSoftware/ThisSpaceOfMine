// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/SessionHandler.hpp>

namespace tsom
{
	NetworkSession::NetworkSession(NetworkReactor& reactor, std::size_t peerId, const Nz::IpAddress& remoteAddress) :
	m_peerId(peerId),
	m_reactor(reactor),
	m_remoteAddress(remoteAddress)
	{
	}

	NetworkSession::~NetworkSession() = default;

	void NetworkSession::Disconnect(DisconnectionType type)
	{
		assert(m_peerId != NetworkReactor::InvalidPeerId);

		m_reactor.DisconnectPeer(m_peerId, 0, type);
	}

	void NetworkSession::HandlePacket(Nz::ByteArray&& byteArray)
	{
		m_sessionHandler->HandlePacket(std::move(byteArray));
	}

	SessionHandler& NetworkSession::SetHandler(std::unique_ptr<SessionHandler>&& sessionHandler)
	{
		m_sessionHandler = std::move(sessionHandler);
		return *m_sessionHandler;
	}
}
