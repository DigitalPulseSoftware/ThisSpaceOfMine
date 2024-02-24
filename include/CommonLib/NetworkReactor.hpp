// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORKREACTOR_HPP
#define TSOM_COMMONLIB_NETWORKREACTOR_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Network/ENetHost.hpp>
#include <concurrentqueue.h>
#include <atomic>
#include <functional>
#include <thread>
#include <variant>
#include <vector>

namespace tsom
{
	enum class DisconnectionType
	{
		Kick,   // DisconnectNow
		Later,  // DisconnectLater
		Normal  // Disconnect
	};

	class TSOM_COMMONLIB_API NetworkReactor
	{
		public:
			struct PeerInfo;
			using PeerInfoCallback = std::function<void (PeerInfo& peerInfo)>;

			NetworkReactor(std::size_t idOffset, Nz::NetProtocol protocol, Nz::UInt16 port, std::size_t maxClient);
			NetworkReactor(const NetworkReactor&) = delete;
			NetworkReactor(NetworkReactor&&) = delete;
			~NetworkReactor();

			std::size_t ConnectTo(Nz::IpAddress address, Nz::UInt32 data = 0);
			void DisconnectPeer(std::size_t peerId, Nz::UInt32 data = 0, DisconnectionType type = DisconnectionType::Normal);

			inline std::size_t GetIdOffset() const;
			inline Nz::NetProtocol GetProtocol() const;

			template<typename ConnectCB, typename DisconnectCB, typename DataCB>
			void Poll(ConnectCB&& onConnection, DisconnectCB&& onDisconnection, DataCB&& onData);

			void QueryInfo(std::size_t peerId, PeerInfoCallback callback);

			void SendData(std::size_t peerId, Nz::UInt8 channelId, Nz::ENetPacketFlags flags, Nz::ByteArray&& payload, std::function<void()> acknowledgeCallback = {});

			NetworkReactor& operator=(const NetworkReactor&) = delete;
			NetworkReactor& operator=(NetworkReactor&&) = delete;

			struct PeerInfo
			{
				Nz::UInt32 ping;
				Nz::UInt32 timeSinceLastReceive;
				Nz::UInt32 totalPacketLost;
				Nz::UInt32 totalPacketReceived;
				Nz::UInt32 totalPacketSent;
				Nz::UInt64 totalByteReceived;
				Nz::UInt64 totalByteSent;
			};

			static constexpr std::size_t InvalidPeerId = std::numeric_limits<std::size_t>::max();

		private:
			void EnsureProperDisconnection(const moodycamel::ProducerToken& producterToken, moodycamel::ConsumerToken& token);
			void HandleConnectionRequests(moodycamel::ConsumerToken& token);
			void ReceivePackets(const moodycamel::ProducerToken& producterToken);
			void SendPackets(const moodycamel::ProducerToken& producterToken, moodycamel::ConsumerToken& token);
			void WorkerThread();

			struct ConnectionRequest
			{
				using Callback = std::function<void(std::size_t clientId)>;

				Callback callback;
				Nz::IpAddress remoteAddress;
				Nz::UInt32 data;
			};

			struct IncomingEvent
			{
				struct ConnectEvent
				{
					bool outgoingConnection;
					Nz::IpAddress remoteAddress;
					Nz::UInt32 data;
				};

				struct DisconnectEvent
				{
					Nz::UInt32 data;
					bool timeout;
				};

				struct PacketEvent
				{
					Nz::ByteArray data;
				};

				struct PeerInfoResponse
				{
					PeerInfo peerInfo;
					PeerInfoCallback callback;
				};

				std::size_t peerId = InvalidPeerId;
				std::variant<ConnectEvent, DisconnectEvent, PacketEvent, PeerInfoResponse> data;
			};

			struct OutgoingEvent
			{
				struct DisconnectEvent
				{
					DisconnectionType type;
					Nz::UInt32 data;
				};

				struct PacketEvent
				{
					Nz::ByteArray data;
					Nz::ENetPacketFlags flags;
					Nz::UInt8 channelId;
					std::function<void()> acknowledgeCallback;
				};

				struct QueryPeerInfo 
				{
					PeerInfoCallback callback;
				};

				std::size_t peerId = InvalidPeerId;
				std::variant<DisconnectEvent, PacketEvent, QueryPeerInfo> data;
			};

			std::atomic_bool m_running;
			std::size_t m_idOffset;
			std::thread m_thread;
			std::vector<Nz::ENetPeer*> m_clients;
			moodycamel::ConcurrentQueue<ConnectionRequest> m_connectionRequests;
			moodycamel::ConcurrentQueue<IncomingEvent> m_incomingQueue;
			moodycamel::ConcurrentQueue<OutgoingEvent> m_outgoingQueue;
			Nz::ENetHost m_host;
			Nz::NetProtocol m_protocol;
	};
}

#include <CommonLib/NetworkReactor.inl>

#endif // TSOM_COMMONLIB_NETWORKREACTOR_HPP
