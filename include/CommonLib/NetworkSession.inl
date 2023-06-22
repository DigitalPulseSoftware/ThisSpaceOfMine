// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	template<typename T>
	void NetworkSession::SendPacket(Nz::UInt8 channelId, Nz::ENetPacketFlags flags, const T& packet)
	{
		static_assert(PacketCount < 0xFF);

		Nz::NetPacket netPacket;
		netPacket << Nz::UInt8(PacketIndex<T>);

		PacketSerializer serializer(packet, true);
		Packets::Serialize(serializer, netPacket);

		m_reactor.SendData(m_peerId, channelId, flags, std::move(netPacket));
	}
}
