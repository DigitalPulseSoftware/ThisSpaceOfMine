// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SESSION_PLAYERSESSIONHANDLER_HPP
#define TSOM_COMMONLIB_SESSION_PLAYERSESSIONHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/ServerPlayer.hpp>
#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API PlayerSessionHandler : public SessionHandler
	{
		public:
			PlayerSessionHandler(NetworkSession* session, ServerPlayer* player);
			~PlayerSessionHandler();

			void HandlePacket(Packets::UpdatePlayerInputs&& playerInputs);

		private:
			ServerPlayer* m_player;
	};
}

#include <CommonLib/Session/PlayerSessionHandler.inl>

#endif
