// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SESSIONHANDLER_HPP
#define TSOM_COMMONLIB_SESSIONHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Network/ENetPacket.hpp>
#include <array>

namespace Nz
{
	class ByteStream;
}

namespace tsom
{
	class NetworkSession;

	class TSOM_COMMONLIB_API SessionHandler
	{
		public:
			using HandlerFunc = void(*)(SessionHandler&, Nz::ByteStream&&);
			using HandlerTable = std::array<HandlerFunc, PacketCount>;
			struct SendAttributes;
			using SendAttributeTable = std::array<SendAttributes, PacketCount>;

			inline SessionHandler(NetworkSession* session);
			SessionHandler(const SessionHandler&) = delete;
			SessionHandler(SessionHandler&&) = delete;
			virtual ~SessionHandler();

			template<typename T> const SendAttributes& GetPacketAttributes();
			inline NetworkSession* GetSession() const;

			void HandlePacket(Nz::ByteArray&& byteArray);

			virtual void OnDeserializationError(std::size_t packetIndex);
			virtual void OnUnexpectedPacket(std::size_t packetIndex);
			virtual void OnUnknownOpcode(Nz::UInt8 opcode);

			SessionHandler& operator=(const SessionHandler&) = delete;
			SessionHandler& operator=(SessionHandler&&) = delete;

			struct SendAttributes
			{
				Nz::UInt8 channelId = InvalidChannel;
				Nz::ENetPacketFlags flags;

				static constexpr Nz::UInt8 InvalidChannel = 0xFF;
			};

			static constexpr SendAttributeTable BuildAttributeTable(std::initializer_list<std::pair<std::size_t, SendAttributes>> initializers);

		protected:
			inline void SetupAttributeTable(const SendAttributeTable& attributeTable);
			template<typename T> void SetupHandlerTable([[maybe_unused]] T*);

		private:
			template<typename T> static constexpr HandlerTable BuildHandlerTable();

			const HandlerTable* m_handlerTable;
			const SendAttributeTable* m_sendAttributes;
			NetworkSession* m_session;
	};
}

#include <CommonLib/SessionHandler.inl>

#endif // TSOM_COMMONLIB_SESSIONHANDLER_HPP
