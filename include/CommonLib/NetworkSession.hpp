// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORKSESSION_HPP
#define TSOM_COMMONLIB_NETWORKSESSION_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <Nazara/Network/ENetPacket.hpp>
#include <Nazara/Network/IpAddress.hpp>

namespace Nz
{
	class ByteArray;
}

namespace tsom
{
	class TSOM_COMMONLIB_API NetworkSession
	{
		public:
			NetworkSession(NetworkReactor& reactor, std::size_t peerId, const Nz::IpAddress& remoteAddress);
			NetworkSession(const NetworkSession&) = delete;
			NetworkSession(NetworkSession&&) = delete;
			~NetworkSession();

			void Disconnect(DisconnectionType type = DisconnectionType::Normal);

			inline std::size_t GetPeerId() const;
			inline Nz::UInt32 GetProtocolVersion() const;
			inline SessionHandler* GetSessionHandler();

			inline bool IsConnected() const;

			void HandlePacket(Nz::ByteArray&& byteArray);

			void QueryInfo(NetworkReactor::PeerInfoCallback callback);

			template<typename T> void SendPacket(const T& packet, std::function<void()> acknowledgeCallback = {});

			SessionHandler& SetHandler(std::unique_ptr<SessionHandler>&& sessionHandler);
			inline void SetProtocolVersion(Nz::UInt32 protocolVersion);

			template<typename T, typename... Args> T& SetupHandler(Args&&... args);

			NetworkSession& operator=(const NetworkSession&) = delete;
			NetworkSession& operator=(NetworkSession&&) = delete;

		private:
			std::size_t m_peerId;
			std::unique_ptr<SessionHandler> m_sessionHandler;
			Nz::IpAddress m_remoteAddress;
			Nz::UInt32 m_protocolVersion;
			NetworkReactor& m_reactor;
	};
}

#include <CommonLib/NetworkSession.inl>

#endif // TSOM_COMMONLIB_NETWORKSESSION_HPP
