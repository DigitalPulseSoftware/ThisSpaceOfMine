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
		Nz::UInt8 opcode;
		netPacket >> opcode;

		(*m_handlerTable)[opcode](*this, std::move(netPacket));
	}

	void SessionHandler::OnUnexpectedPacket()
	{
		fmt::print("Received unexpected packet\n");
	}
}
