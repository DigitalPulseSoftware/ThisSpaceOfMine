// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
#define TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientSessionHandler : public SessionHandler
	{
		public:
			ClientSessionHandler(NetworkSession* session);
			~ClientSessionHandler() = default;

			void HandlePacket(Packets::AuthResponse&& authResponse);
	};
}

#include <ClientLib/ClientSessionHandler.inl>

#endif
