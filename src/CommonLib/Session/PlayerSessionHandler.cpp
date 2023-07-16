// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Session/PlayerSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::EntitiesCreation>,    { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesDelete>,      { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesStateUpdate>, { 1, 0 } },
		{ PacketIndex<Packets::PlayerJoin>,          { 0, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::PlayerLeave>,         { 0, Nz::ENetPacketFlag_Reliable } }
	});

	PlayerSessionHandler::PlayerSessionHandler(NetworkSession* session, ServerPlayer* player) :
	SessionHandler(session),
	m_player(player)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
	}

	PlayerSessionHandler::~PlayerSessionHandler()
	{
		m_player->Destroy();
	}

	void PlayerSessionHandler::HandlePacket(Packets::UpdatePlayerInputs&& playerInputs)
	{
		m_player->HandleInputs(playerInputs.inputs);
	}
}
