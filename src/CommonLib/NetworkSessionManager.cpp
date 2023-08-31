// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <cassert>

namespace tsom
{
	void NetworkSessionManager::Poll()
	{
		auto ConnectionHandler = [&]([[maybe_unused]] bool outgoingConnection, std::size_t peerIndex, const Nz::IpAddress& remoteAddress, [[maybe_unused]] Nz::UInt32 data)
		{
			assert(!outgoingConnection);
			assert(!m_sessions[peerIndex].has_value());
			assert(data == 0);

			fmt::print("Peer connected (outgoing: {}, peerIndex: {}, address: {}, data: {})\n", outgoingConnection, peerIndex, fmt::streamed(remoteAddress), data);
			m_sessions[peerIndex].emplace(m_reactor, peerIndex, remoteAddress);
			m_sessions[peerIndex]->SetHandler(m_handlerFactory(&m_sessions[peerIndex].value()));
		};

		auto DisconnectionHandler = [&](std::size_t peerIndex, [[maybe_unused]] Nz::UInt32 data, bool timeout)
		{
			assert(m_sessions[peerIndex].has_value());
			assert(data == 0);

			fmt::print("Peer {} (peerIndex: {}, data: {})\n", (timeout) ? "timeout" : "disconnected", peerIndex, data);
			m_sessions[peerIndex].reset();
		};

		auto PacketHandler = [&](std::size_t peerIndex, Nz::NetPacket&& packet)
		{
			assert(m_sessions[peerIndex].has_value());

			m_sessions[peerIndex]->HandlePacket(std::move(packet));
		};

		m_reactor.Poll(ConnectionHandler, DisconnectionHandler, PacketHandler);
	}
}
