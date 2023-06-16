// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline std::size_t NetworkReactor::GetIdOffset() const
	{
		return m_idOffset;
	}

	inline Nz::NetProtocol NetworkReactor::GetProtocol() const
	{
		return m_protocol;
	}

	template<typename ConnectCB, typename DisconnectCB, typename DataCB>
	void NetworkReactor::Poll(ConnectCB&& onConnection, DisconnectCB&& onDisconnection, DataCB&& onData)
	{
		IncomingEvent inEvent;
		while (m_incomingQueue.try_dequeue(inEvent))
		{
			std::visit([&](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, IncomingEvent::ConnectEvent>)
				{
					onConnection(arg.outgoingConnection, inEvent.peerId, arg.remoteAddress, arg.data);
				}
				else if constexpr (std::is_same_v<T, IncomingEvent::DisconnectEvent>)
				{
					onDisconnection(inEvent.peerId, arg.data);
				}
				else if constexpr (std::is_same_v<T, IncomingEvent::PacketEvent>)
				{
					onData(inEvent.peerId, std::move(arg.packet));
				}
				else if constexpr (std::is_same_v<T, IncomingEvent::PeerInfoResponse>)
				{
					arg.callback(arg.peerInfo);
				}
				else
					static_assert(Nz::AlwaysFalse<T>::value, "non-exhaustive visitor");

			}, inEvent.data);
		}
	}
}
