// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/PlayerSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable m_packetFlags = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::NetworkStrings>, { 0, Nz::ENetPacketFlag_Reliable } }
	});

	PlayerSessionHandler::PlayerSessionHandler(NetworkSession* session) :
	SessionHandler(session)
	{
		SetupHandlerTable<PlayerSessionHandler>();
	}

	void PlayerSessionHandler::HandlePacket(Packets::Test&& test)
	{
		fmt::print("Received Test: {}\n", test.str);
	}
}
