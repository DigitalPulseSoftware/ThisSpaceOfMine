// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include "NetworkSession.hpp"

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

	template<typename T>
	void NetworkSession::SendPacket(Nz::UInt8 channelId, Nz::ENetPacketFlags flags, const T& packet)
	{
		static_assert(PacketCount < 0xFF);

		Nz::NetPacket netPacket;
		netPacket << Nz::UInt8(PacketIndex<T>);

		PacketSerializer serializer(netPacket, true);
		Packets::Serialize(serializer, const_cast<T&>(packet));

		netPacket.FlushBits();

		m_reactor.SendData(m_peerId, channelId, flags, std::move(netPacket));
	}
}
