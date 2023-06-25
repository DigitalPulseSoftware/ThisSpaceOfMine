// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORKSESSION_HPP
#define TSOM_COMMONLIB_NETWORKSESSION_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Network/ENetPacket.hpp>

namespace Nz
{
	class NetPacket;
}

namespace tsom
{
	class SessionHandler;

	class TSOM_COMMONLIB_API NetworkSession
	{
		public:
			NetworkSession(NetworkReactor& reactor, std::size_t peerId, const Nz::IpAddress& remoteAddress);
			NetworkSession(const NetworkSession&) = delete;
			NetworkSession(NetworkSession&&) = delete;
			~NetworkSession();

			void Disconnect();

			inline std::size_t GetPeerId() const;
			inline SessionHandler* GetSessionHandler();

			void HandlePacket(Nz::NetPacket&& netPacket);

			template<typename T> void SendPacket(Nz::UInt8 channelId, Nz::ENetPacketFlags flags, const T& packet);

			void SetHandler(std::unique_ptr<SessionHandler>&& sessionHandler);

			NetworkSession& operator=(const NetworkSession&) = delete;
			NetworkSession& operator=(NetworkSession&&) = delete;

		private:
			std::size_t m_peerId;
			std::unique_ptr<SessionHandler> m_sessionHandler;
			NetworkReactor& m_reactor;
			Nz::IpAddress m_remoteAddress;
	};
}

#include <CommonLib/NetworkSession.inl>

#endif
