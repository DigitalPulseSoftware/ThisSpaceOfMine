// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <NazaraUtils/Hash.hpp>
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

			std::string addressStr = remoteAddress.ToString(false);

			fmt::print("Peer connected (outgoing: {}, peerIndex: {}, hashed address: {:x}, data: {})\n", outgoingConnection, peerIndex, Nz::FNV1a64(addressStr), data);
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

		auto PacketHandler = [&](std::size_t peerIndex, Nz::ByteArray&& packet)
		{
			assert(m_sessions[peerIndex].has_value());

			m_sessions[peerIndex]->HandlePacket(std::move(packet));
		};

		m_reactor.Poll(ConnectionHandler, DisconnectionHandler, PacketHandler);
	}
}
