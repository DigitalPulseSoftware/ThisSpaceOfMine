// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORKSESSION_HPP
#define TSOM_COMMONLIB_NETWORKSESSION_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <Nazara/Network/IpAddress.hpp>

namespace Nz
{
	class NetPacket;
}

namespace tsom
{
	class NetworkSessionManager;

	class TSOM_COMMONLIB_API NetworkSession
	{
		public:
			inline NetworkSession(NetworkSessionManager& owner, std::size_t peerId, const Nz::IpAddress& remoteAddres);
			NetworkSession(const NetworkSession&) = delete;
			NetworkSession(NetworkSession&&) = delete;
			~NetworkSession() = default;

			inline void HandlePacket(Nz::NetPacket&& netPacket);

			inline void SetHandler(std::unique_ptr<SessionHandler>&& sessionHandler);

			NetworkSession& operator=(const NetworkSession&) = delete;
			NetworkSession& operator=(NetworkSession&&) = delete;

		private:
			std::size_t m_peerId;
			std::unique_ptr<SessionHandler> m_sessionHandler;
			NetworkSessionManager& m_owner;
			Nz::IpAddress m_remoteAddress;
	};
}

#include <CommonLib/NetworkSession.inl>

#endif
