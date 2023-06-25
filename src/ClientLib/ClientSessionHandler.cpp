// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable m_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthRequest>, { 0, Nz::ENetPacketFlag_Reliable } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session) :
	SessionHandler(session)
	{
		SetupHandlerTable<ClientSessionHandler>();
		SetupAttributeTable(m_packetAttributes);
	}

	void ClientSessionHandler::HandlePacket(Packets::AuthResponse&& authResponse)
	{
		fmt::print("Auth response");
	}
}
