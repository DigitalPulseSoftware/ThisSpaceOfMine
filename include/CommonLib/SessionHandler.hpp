// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SESSIONHANDLER_HPP
#define TSOM_COMMONLIB_SESSIONHANDLER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Network/ENetPacket.hpp>
#include <array>

namespace Nz
{
	class NetPacket;
}

namespace tsom
{
	class NetworkSession;

	class TSOM_COMMONLIB_API SessionHandler
	{
		public:
			using HandlerFunc = void(*)(SessionHandler&, Nz::NetPacket&&);
			using HandlerTable = std::array<HandlerFunc, PacketCount>;
			struct SendAttributes;
			using SendAttributeTable = std::array<SendAttributes, PacketCount>;

			inline SessionHandler(NetworkSession* session);
			SessionHandler(const SessionHandler&) = delete;
			SessionHandler(SessionHandler&&) = delete;
			virtual ~SessionHandler();

			void HandlePacket(Nz::NetPacket&& netPacket);

			virtual void OnUnexpectedPacket(std::size_t packetIndex);

			template<typename T> void SendPacket(const T& packet);

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
			template<typename T> void SetupHandlerTable();

		private:
			template<typename T> static constexpr HandlerTable BuildHandlerTable();

			const HandlerTable* m_handlerTable;
			const SendAttributeTable* m_sendAttributes;
			NetworkSession* m_session;
	};
}

#include <CommonLib/SessionHandler.inl>

#endif
