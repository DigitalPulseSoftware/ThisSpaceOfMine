// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <NazaraUtils/Hash.hpp>
#include <spdlog/spdlog.h>
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

			spdlog::info("Peer connected (peerIndex: {}, hashed address: {:x})", peerIndex, Nz::FNV1a64(addressStr));
			m_sessions[peerIndex].emplace(m_reactor, peerIndex, remoteAddress);
			m_sessions[peerIndex]->SetHandler(m_handlerFactory(&m_sessions[peerIndex].value()));
		};

		auto DisconnectionHandler = [&](std::size_t peerIndex, [[maybe_unused]] Nz::UInt32 data, bool timeout)
		{
			assert(m_sessions[peerIndex].has_value());
			assert(data == 0);

			spdlog::info("Peer {} (peerIndex: {})", (timeout) ? "timeout" : "disconnected", peerIndex);
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
