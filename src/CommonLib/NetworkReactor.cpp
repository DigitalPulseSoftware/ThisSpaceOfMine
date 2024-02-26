// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Core/ThreadExt.hpp>
#include <cassert>
#include <stdexcept>

namespace tsom
{
	NetworkReactor::NetworkReactor(std::size_t idOffset, Nz::NetProtocol protocol, Nz::UInt16 port, std::size_t maxClient) :
	m_idOffset(idOffset),
	m_protocol(protocol)
	{
		if (port > 0)
		{
			if (!m_host.Create(protocol, port, maxClient, Constants::NetworkChannelCount))
				throw std::runtime_error("failed to start reactor");
		}
		else if (!m_host.Create((protocol == Nz::NetProtocol::IPv4) ? Nz::IpAddress::LoopbackIpV4 : Nz::IpAddress::LoopbackIpV6, maxClient, Constants::NetworkChannelCount))
			throw std::runtime_error("failed to start reactor");

		m_clients.resize(maxClient, nullptr);

		m_running.store(true, std::memory_order_release);
		m_thread = std::thread(&NetworkReactor::WorkerThread, this);
	}

	NetworkReactor::~NetworkReactor()
	{
		m_running.store(false, std::memory_order_relaxed);
		m_thread.join();
	}

	std::size_t NetworkReactor::ConnectTo(Nz::IpAddress address, Nz::UInt32 data)
	{
		ConnectionRequest request;
		request.data = data;
		request.remoteAddress = std::move(address);

		// We will need a few synchronization primitives to block the calling thread until the reactor has treated our request
		std::size_t newClientId = InvalidPeerId;
		// As InvalidClientId is a possible return from the callback, we need another variable to prevent spurious wakeup
		std::atomic_bool hasReturned = false;
		request.callback = [&](std::size_t peerId)
		{
			// This callback is called from within the reactor
			newClientId = m_idOffset + peerId;
			hasReturned = true;
			hasReturned.notify_all();
		};
		m_connectionRequests.enqueue(request);

		hasReturned.wait(false);

		return newClientId;
	}

	void NetworkReactor::DisconnectPeer(std::size_t peerId, Nz::UInt32 data, DisconnectionType type)
	{
		assert(peerId >= m_idOffset);

		OutgoingEvent::DisconnectEvent disconnectEvent;
		disconnectEvent.data = data;
		disconnectEvent.type = type;

		OutgoingEvent outgoingData;
		outgoingData.peerId = peerId - m_idOffset;
		outgoingData.data = std::move(disconnectEvent);

		m_outgoingQueue.enqueue(std::move(outgoingData));
	}

	void NetworkReactor::QueryInfo(std::size_t peerId, PeerInfoCallback callback)
	{
		assert(peerId >= m_idOffset);
		assert(callback);

		OutgoingEvent outgoingRequest;
		outgoingRequest.peerId = peerId - m_idOffset;
		auto& queryInfo = outgoingRequest.data.emplace<OutgoingEvent::QueryPeerInfo>();
		queryInfo.callback = std::move(callback);

		m_outgoingQueue.enqueue(std::move(outgoingRequest));
	}

	void NetworkReactor::SendData(std::size_t peerId, Nz::UInt8 channelId, Nz::ENetPacketFlags flags, Nz::ByteArray&& payload, std::function<void()> acknowledgeCallback)
	{
		assert(peerId >= m_idOffset);

		OutgoingEvent::PacketEvent packetEvent;
		packetEvent.acknowledgeCallback = std::move(acknowledgeCallback);
		packetEvent.channelId = channelId;
		packetEvent.data = std::move(payload);
		packetEvent.flags = flags;

		OutgoingEvent outgoingData;
		outgoingData.peerId = peerId - m_idOffset;
		outgoingData.data = std::move(packetEvent);

		m_outgoingQueue.enqueue(std::move(outgoingData));
	}

	void NetworkReactor::WorkerThread()
	{
		Nz::SetCurrentThreadName("NetworkReactor");

		moodycamel::ConsumerToken connectionToken(m_connectionRequests);
		moodycamel::ConsumerToken outgoingToken(m_outgoingQueue);
		moodycamel::ProducerToken incomingToken(m_incomingQueue);

		while (m_running.load(std::memory_order_acquire))
		{
			ReceivePackets(incomingToken);
			SendPackets(incomingToken, outgoingToken);

			// Handle connection requests last to treat disconnection request before connection requests
			HandleConnectionRequests(connectionToken);
		}

		EnsureProperDisconnection(incomingToken, outgoingToken);
	}

	void NetworkReactor::EnsureProperDisconnection(const moodycamel::ProducerToken& producterToken, moodycamel::ConsumerToken& token)
	{
		// Prevent someone connecting from now
		if (Nz::IpAddress listenAddress = m_host.GetBoundAddress(); listenAddress.IsValid() && !listenAddress.IsLoopback())
			m_host.AllowsIncomingConnections(false);

		// Send every pending packet and handle disconnection requests
		SendPackets(producterToken, token);

		// Then, force a disconnection for every remaining peer
		for (Nz::ENetPeer* peer : m_clients)
		{
			if (!peer)
				continue;

			switch (peer->GetState())
			{
				case Nz::ENetPeerState::AcknowledgingDisconnect:
				case Nz::ENetPeerState::Disconnected:
				case Nz::ENetPeerState::Disconnecting:
				case Nz::ENetPeerState::Zombie:
					break;

				default:
					peer->Disconnect(0); //< FIXME: DisconnectLater doesn't seem to work here
					break;
			}
		}

		// Use a timeout to prevent hanging on silent peers
		Nz::MillisecondClock c;
		while (c.GetElapsedTime() < Nz::Time::Milliseconds(1000))
		{
			Nz::ENetEvent event;
			if (m_host.Service(&event, 1) > 0)
			{
				switch (event.type)
				{
					case Nz::ENetEventType::Disconnect:
					case Nz::ENetEventType::DisconnectTimeout:
					{
						Nz::UInt16 peerId = event.peer->GetPeerId();
						m_clients[peerId] = nullptr;
						break;
					}

					default:
						// Ignore everything else
						break;
				}
			}

			// Exit when every client has properly disconnected
			if (std::all_of(m_clients.begin(), m_clients.end(), [](Nz::ENetPeer* peer) { return peer == nullptr; }))
				break;
		}
	}

	void NetworkReactor::HandleConnectionRequests(moodycamel::ConsumerToken& token)
{
		ConnectionRequest request;
		while (m_connectionRequests.try_dequeue(token, request))
		{
			if (Nz::ENetPeer* peer = m_host.Connect(request.remoteAddress, Constants::NetworkChannelCount, request.data))
			{
				Nz::UInt16 peerId = peer->GetPeerId();
				m_clients[peerId] = peer;

				request.callback(peerId);
			}
			else
				request.callback(InvalidPeerId);
		}
	}

	void NetworkReactor::ReceivePackets(const moodycamel::ProducerToken& producterToken)
	{
		Nz::ENetEvent event;
		if (m_host.Service(&event, 5) > 0)
		{
			do
			{
				switch (event.type)
				{
					case Nz::ENetEventType::Disconnect:
					case Nz::ENetEventType::DisconnectTimeout:
					{
						Nz::UInt16 peerId = event.peer->GetPeerId();
						m_clients[peerId] = nullptr;

						IncomingEvent::DisconnectEvent disconnectEvent;
						disconnectEvent.data = event.data;
						disconnectEvent.timeout = (event.type == Nz::ENetEventType::DisconnectTimeout);

						IncomingEvent newEvent;
						newEvent.peerId = m_idOffset + peerId;
						newEvent.data.emplace<IncomingEvent::DisconnectEvent>(std::move(disconnectEvent));

						m_incomingQueue.enqueue(producterToken, std::move(newEvent));
						break;
					}

					case Nz::ENetEventType::IncomingConnect:
					case Nz::ENetEventType::OutgoingConnect:
					{
						Nz::UInt16 peerId = event.peer->GetPeerId();
						m_clients[peerId] = event.peer;

						IncomingEvent::ConnectEvent connectEvent;
						connectEvent.data = event.data;
						connectEvent.remoteAddress = event.peer->GetAddress();
						connectEvent.outgoingConnection = (event.type == Nz::ENetEventType::OutgoingConnect);

						IncomingEvent newEvent;
						newEvent.peerId = m_idOffset + peerId;
						newEvent.data.emplace<IncomingEvent::ConnectEvent>(std::move(connectEvent));

						m_incomingQueue.enqueue(producterToken, std::move(newEvent));
						break;
					}

					case Nz::ENetEventType::Receive:
					{
						Nz::UInt16 peerId = event.peer->GetPeerId();

						IncomingEvent::PacketEvent packetEvent;
						packetEvent.data = std::move(event.packet->data);

						IncomingEvent newEvent;
						newEvent.peerId = m_idOffset + peerId;
						newEvent.data.emplace<IncomingEvent::PacketEvent>(std::move(packetEvent));

						m_incomingQueue.enqueue(producterToken, std::move(newEvent));
						break;
					}

					default:
						break;
				}
			}
			while (m_host.CheckEvents(&event));
		}
	}

	void NetworkReactor::SendPackets(const moodycamel::ProducerToken& producterToken, moodycamel::ConsumerToken& token)
	{
		OutgoingEvent outEvent;
		while (m_outgoingQueue.try_dequeue(token, outEvent))
		{
			std::visit([&](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, OutgoingEvent::DisconnectEvent>)
				{
					if (Nz::ENetPeer* peer = m_clients[outEvent.peerId])
					{
						switch (arg.type)
						{
							case DisconnectionType::Kick:
							{
								peer->DisconnectNow(arg.data);

								// DisconnectNow does not generate Disconnect event
								m_clients[outEvent.peerId] = nullptr;

								IncomingEvent newEvent;
								newEvent.peerId = m_idOffset + outEvent.peerId;

								auto& disconnectEvent = newEvent.data.emplace<IncomingEvent::DisconnectEvent>();
								disconnectEvent.data = 0;

								m_incomingQueue.enqueue(producterToken, std::move(newEvent));
								break;
							}

							case DisconnectionType::Later:
								peer->DisconnectLater(arg.data);
								break;

							case DisconnectionType::Normal:
								peer->Disconnect(arg.data);
								break;

							default:
								assert(!"Unknown disconnection type");
								break;
						}
					}
				}
				else if constexpr (std::is_same_v<T, OutgoingEvent::PacketEvent>)
				{
					if (Nz::ENetPeer* peer = m_clients[outEvent.peerId])
					{
						Nz::ENetPacketRef packet = m_host.AllocatePacket(arg.flags, std::move(arg.data));
						if (arg.acknowledgeCallback)
							packet->OnAcknowledged.Connect(std::move(arg.acknowledgeCallback));

						peer->Send(arg.channelId, std::move(packet));
					}
				}
				else if constexpr (std::is_same_v<T, OutgoingEvent::QueryPeerInfo>)
				{
					if (Nz::ENetPeer* peer = m_clients[outEvent.peerId])
					{
						IncomingEvent newEvent;
						newEvent.peerId = m_idOffset + outEvent.peerId;

						auto& peerInfo = newEvent.data.emplace<IncomingEvent::PeerInfoResponse>();
						peerInfo.callback = std::move(arg.callback);
						peerInfo.peerInfo.timeSinceLastReceive = m_host.GetServiceTime() - peer->GetLastReceiveTime();
						peerInfo.peerInfo.ping = peer->GetRoundTripTime();
						peerInfo.peerInfo.totalByteReceived = peer->GetTotalByteReceived();
						peerInfo.peerInfo.totalByteSent = peer->GetTotalByteSent();
						peerInfo.peerInfo.totalPacketLost = peer->GetTotalPacketLost();
						peerInfo.peerInfo.totalPacketReceived = peer->GetTotalPacketReceived();
						peerInfo.peerInfo.totalPacketSent = peer->GetTotalPacketSent();

						m_incomingQueue.enqueue(producterToken, std::move(newEvent));
					}
				}
				else
					static_assert(Nz::AlwaysFalse<T>::value, "non-exhaustive visitor");

			}, outEvent.data);
		}
	}
}
