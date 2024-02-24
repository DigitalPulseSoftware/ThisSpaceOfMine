// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/ErrorFlags.hpp>
#include <fmt/format.h>
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
					return [](SessionHandler& sessionHandler, Nz::ByteStream&& packet)
					{
						PacketType unserializedPacket;

						PacketSerializer serializer(packet, false);
						try
						{
							Nz::ErrorFlags errFlags(Nz::ErrorMode::Silent | Nz::ErrorMode::ThrowException);
							Packets::Serialize(serializer, unserializedPacket);
						}
						catch (const std::exception&)
						{
							constexpr std::size_t packetIndex = Nz::TypeListFind<PacketTypes, PacketType>;
							static_cast<Handler&>(sessionHandler).OnDeserializationError(packetIndex);
							return;
						}

						static_cast<Handler&>(sessionHandler).HandlePacket(std::move(unserializedPacket));
					};
				}
				else
				{
					return [](SessionHandler& sessionHandler, Nz::ByteStream&& /*packet*/)
					{
						constexpr std::size_t packetIndex = Nz::TypeListFind<PacketTypes, PacketType>;
						static_cast<Handler&>(sessionHandler).OnUnexpectedPacket(packetIndex);
					};
				}
			}
		};
	}

	inline SessionHandler::SessionHandler(NetworkSession* session) :
	m_session(session)
	{
	}

	template<typename T>
	auto SessionHandler::GetPacketAttributes() -> const SendAttributes&
	{
		const SendAttributes& sendAttributes = (*m_sendAttributes)[PacketIndex<T>];
		if (sendAttributes.channelId == SendAttributes::InvalidChannel)
			throw std::runtime_error(fmt::format("missing packet setup for {}", PacketNames[PacketIndex<T>]));

		return sendAttributes;
	}

	inline NetworkSession* SessionHandler::GetSession() const
	{
		return m_session;
	}

	constexpr auto SessionHandler::BuildAttributeTable(std::initializer_list<std::pair<std::size_t, SendAttributes>> initializers) -> SendAttributeTable
	{
		SendAttributeTable attributeTables;
		for (auto&& [packetIndex, attributes] : initializers)
			attributeTables[packetIndex] = attributes;

		return attributeTables;
	}

	inline void SessionHandler::SetupAttributeTable(const SendAttributeTable& attributeTable)
	{
		m_sendAttributes = &attributeTable;
	}

	template<typename T>
	void SessionHandler::SetupHandlerTable([[maybe_unused]] T*)
	{
		static constexpr HandlerTable handlerTable = BuildHandlerTable<T>();
		m_handlerTable = &handlerTable;
	}

	template<typename T>
	constexpr SessionHandler::HandlerTable SessionHandler::BuildHandlerTable()
	{
		HandlerTable handlerTable;
#define TSOM_NETWORK_PACKET(Name) handlerTable[PacketIndex<Packets::Name>] = Helper::HandlerBuilder<T, Packets::Name>{}();
#include <CommonLib/Protocol/PacketList.hpp>

		return handlerTable;
	}
}
