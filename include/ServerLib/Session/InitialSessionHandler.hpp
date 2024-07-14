// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SESSION_INITIALSESSIONHANDLER_HPP
#define TSOM_SERVERLIB_SESSION_INITIALSESSIONHANDLER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	class ServerInstance;

	class TSOM_SERVERLIB_API InitialSessionHandler : public SessionHandler
	{
		public:
			InitialSessionHandler(ServerInstance& instance, NetworkSession* session);
			~InitialSessionHandler() = default;

			void HandlePacket(Packets::AuthRequest&& authRequest);

			void OnDeserializationError(std::size_t packetIndex) override;
			void OnUnexpectedPacket(std::size_t packetIndex) override;
			void OnUnknownOpcode(Nz::UInt8 opcode) override;

		private:
			ServerInstance& m_instance;
	};
}

#include <ServerLib/Session/InitialSessionHandler.inl>

#endif // TSOM_SERVERLIB_SESSION_INITIALSESSIONHANDLER_HPP
