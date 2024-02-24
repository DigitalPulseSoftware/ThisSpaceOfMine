// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <Nazara/Core/ByteArray.hpp>
#include <fmt/format.h>

namespace tsom
{
	SessionHandler::~SessionHandler() = default;

	Nz::UInt32 SessionHandler::GetProtocolVersion() const
	{
		return m_session->GetProtocolVersion();
	}

	void SessionHandler::HandlePacket(Nz::ByteArray&& byteArray)
	{
		const HandlerTable& handlerTable = *m_handlerTable;

		Nz::ByteStream byteStream(&byteArray, Nz::OpenMode::Read);

		Nz::UInt8 opcode;
		try
		{
			byteStream >> opcode;
		}
		catch (const std::exception& e)
		{
			OnUnknownOpcode(0xFF);
			return;
		}

		if (opcode >= handlerTable.size())
		{
			OnUnknownOpcode(opcode);
			return;
		}

		(*m_handlerTable)[opcode](*this, std::move(byteStream));
	}

	void SessionHandler::OnDeserializationError(std::size_t packetIndex)
	{
		fmt::print("Serialization error of packet of type {}\n", PacketNames[packetIndex]);
	}

	void SessionHandler::OnUnexpectedPacket(std::size_t packetIndex)
	{
		fmt::print("Received unexpected packet of type {}\n", PacketNames[packetIndex]);
	}

	void SessionHandler::OnUnknownOpcode(Nz::UInt8 opcode)
	{
		fmt::print("Received packet with unknown opcode {}\n", +opcode);
	}
}
