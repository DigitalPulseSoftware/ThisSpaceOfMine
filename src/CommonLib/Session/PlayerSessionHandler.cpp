// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Session/PlayerSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable m_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::EntitiesCreation>,    { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesDelete>,      { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesStateUpdate>, { 1, 0 } }
	});

	PlayerSessionHandler::PlayerSessionHandler(NetworkSession* session, ServerPlayer* player) :
	SessionHandler(session),
	m_player(player)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(m_packetAttributes);
	}

	void PlayerSessionHandler::HandlePacket(Packets::UpdatePlayerInputs&& playerInputs)
	{
	}
}
