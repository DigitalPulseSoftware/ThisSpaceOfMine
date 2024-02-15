// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <type_traits>

namespace tsom
{
	inline NetworkSessionManager::NetworkSessionManager(Nz::UInt16 port, Nz::NetProtocol protocol, std::size_t maxSessions) :
	m_sessions(maxSessions),
	m_handlerFactory(nullptr),
	m_reactor(0, protocol, port, maxSessions)
	{
	}

	void NetworkSessionManager::SendData(std::size_t peerId, Nz::UInt8 channelId, Nz::ENetPacketFlags flags, Nz::NetPacket&& packet)
	{
		m_reactor.SendData(peerId, channelId, flags, std::move(packet));
	}

	template<typename T, typename... Args>
	void NetworkSessionManager::SetDefaultHandler(Args&&... args)
	{
		static_assert(std::is_base_of_v<SessionHandler, T>);

		m_handlerFactory = [=](NetworkSession* session) mutable -> std::unique_ptr<SessionHandler> { return std::make_unique<T>(std::forward<Args>(args)..., session); };
	}
}
