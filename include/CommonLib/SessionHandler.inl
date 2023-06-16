// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <type_traits>

namespace tsom
{
	namespace Helper
	{
		template<typename, typename, typename = void>
		struct HasHandlePacket : std::false_type {};

		template<typename Handler, typename PacketType>
		struct HasHandlePacket<Handler, PacketType, std::void_t<decltype(std::declval<Handler>().HandlePacket(std::declval<PacketType>()))>> : std::true_type {};

		template<typename Handler, typename PacketType>
		struct HandlerBuilder
		{
			static_assert(std::is_base_of_v<SessionHandler, Handler>);

			constexpr SessionHandler::HandlerFunc operator()()
			{
				if constexpr (HasHandlePacket<Handler, PacketType>())
				{
					return [](SessionHandler& sessionHandler, Nz::NetPacket&& packet)
					{
						PacketType unserializedPacket;

						PacketSerializer serializer(packet, false);
						Packets::Serialize(serializer, unserializedPacket);

						static_cast<Handler&>(sessionHandler).HandlePacket(std::move(unserializedPacket));
					};
				}
				else
				{
					return [](SessionHandler& sessionHandler, Nz::NetPacket&& packet)
					{
						static_cast<Handler&>(sessionHandler).OnUnexpectedPacket();
					};
				}
			}
		};
	}

	template<typename T>
	void SessionHandler::SetupHandlerTable()
	{
		static constexpr HandlerTable handlerTable = BuildHandlerTable<T>();
		m_handlerTable = &handlerTable;
	}

	template<typename T>
	constexpr SessionHandler::HandlerTable SessionHandler::BuildHandlerTable()
	{
		HandlerTable handlerTable;
#define TSOM_NETWORK_PACKET(Name) handlerTable[Nz::TypeListFind<PacketTypes, Packets::Name>] = Helper::HandlerBuilder<T, Packets::Name>{}();
#include <CommonLib/Protocol/PacketList.hpp>

		return handlerTable;
	}
}
