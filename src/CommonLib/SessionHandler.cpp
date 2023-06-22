// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/SessionHandler.hpp>
#include <Nazara/Network/NetPacket.hpp>
#include <fmt/format.h>

namespace tsom
{
	SessionHandler::~SessionHandler() = default;

	void SessionHandler::HandlePacket(Nz::NetPacket&& netPacket)
	{
		const HandlerTable& handlerTable = *m_handlerTable;

		Nz::UInt8 opcode;
		netPacket >> opcode;

		if (opcode >= handlerTable.size())
		{
			fmt::print("Received corrupted packet\n");
			return;
		}

		(*m_handlerTable)[opcode](*this, std::move(netPacket));
	}

	void SessionHandler::OnUnexpectedPacket(std::size_t packetIndex)
	{
		fmt::print("Received unexpected packet of type {}\n", PacketNames[packetIndex]);
	}
}
