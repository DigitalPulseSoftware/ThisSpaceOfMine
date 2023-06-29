// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORK_PACKETS_HPP
#define TSOM_COMMONLIB_NETWORK_PACKETS_HPP

#include <NazaraUtils/TypeList.hpp>
#include <CommonLib/Export.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Protocol/CompressedInteger.hpp>
#include <CommonLib/Protocol/PacketSerializer.hpp>

namespace tsom
{
	namespace Packets
	{
#define TSOM_NETWORK_PACKET(Name) struct Name;
#include <CommonLib/Protocol/PacketList.hpp>
	}

	using PacketTypes = Nz::TypeList<
#define TSOM_NETWORK_PACKET(Name) Packets::Name,
#define TSOM_NETWORK_PACKET_LAST(Name) Packets::Name
#include <CommonLib/Protocol/PacketList.hpp>
	>;

	static constexpr std::size_t PacketCount = Nz::TypeListSize<PacketTypes>;

	template<typename T> static constexpr std::size_t PacketIndex = Nz::TypeListFind<PacketTypes, T>;

	extern std::array<std::string_view, PacketCount> PacketNames;

	namespace Packets
	{
		namespace Helper
		{
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerInputs& data);
		}

		struct AuthRequest
		{
			std::string nickname;
		};

		struct AuthResponse
		{
			bool succeeded;
		};

		struct NetworkStrings
		{
			CompressedUnsigned<Nz::UInt32> startId;
			std::vector<std::string> strings;
		};

		struct UpdatePlayerInputs
		{
			PlayerInputs inputs;
		};

		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, AuthRequest& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, AuthResponse& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, NetworkStrings& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data);
	}
}

#include <CommonLib/Protocol/Packets.inl>

#endif
