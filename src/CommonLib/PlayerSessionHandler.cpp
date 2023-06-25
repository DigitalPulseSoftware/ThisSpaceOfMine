// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/PlayerSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable m_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthResponse>, { 0, Nz::ENetPacketFlag_Reliable } }
	});

	PlayerSessionHandler::PlayerSessionHandler(NetworkSession* session) :
	SessionHandler(session)
	{
		SetupHandlerTable<PlayerSessionHandler>();
		SetupAttributeTable(m_packetAttributes);
	}

	void PlayerSessionHandler::HandlePacket(Packets::AuthRequest&& authRequest)
	{
		fmt::print("auth request from {}\n", authRequest.nickname);

		Packets::AuthResponse response;
		response.succeeded = true;

		SendPacket(response);
	}
}
