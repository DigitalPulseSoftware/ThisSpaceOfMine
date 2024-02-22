// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	inline std::size_t NetworkSession::GetPeerId() const
	{
		return m_peerId;
	}

	inline SessionHandler* NetworkSession::GetSessionHandler()
	{
		return m_sessionHandler.get();
	}

	inline bool NetworkSession::IsConnected() const
	{
		return m_peerId != NetworkReactor::InvalidPeerId;
	}

	template<typename T>
	void NetworkSession::SendPacket(const T& packet, std::function<void()> acknowledgeCallback)
	{
		static_assert(PacketCount < 0xFF);

		const SessionHandler::SendAttributes& sendAttributes = m_sessionHandler->GetPacketAttributes<T>();

		Nz::NetPacket netPacket;
		netPacket << Nz::UInt8(PacketIndex<T>);

		PacketSerializer serializer(netPacket, true);
		Packets::Serialize(serializer, const_cast<T&>(packet));

		netPacket.FlushBits();

		m_reactor.SendData(m_peerId, sendAttributes.channelId, sendAttributes.flags, std::move(netPacket), std::move(acknowledgeCallback));
	}

	template<typename T, typename ...Args>
	T& NetworkSession::SetupHandler(Args&& ...args)
	{
		return static_cast<T&>(SetHandler(std::make_unique<T>(this, std::forward<Args>(args)...)));
	}
}
