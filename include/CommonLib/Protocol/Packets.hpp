// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORK_PACKETS_HPP
#define TSOM_COMMONLIB_NETWORK_PACKETS_HPP

#include <NazaraUtils/TypeList.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
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
			using EntityId = Nz::UInt16;

			struct EntityState
			{
				Nz::Quaternionf rotation;
				Nz::Vector3f position;
			};

			struct PlayerEntityData
			{
				std::string nickname;
			};

			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityState& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerEntityData& data);
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

		struct EntitiesCreation
		{
			struct EntityData
			{
				Helper::EntityId entityId;
				Helper::EntityState initialStates;
				std::variant<
					Helper::PlayerEntityData
				> data;
			};

			std::vector<EntityData> entities;
		};

		struct EntitiesDelete
		{
			std::vector<Helper::EntityId> entities;
		};

		struct EntitiesStateUpdate
		{
			struct EntityData
			{
				Helper::EntityId entityId;
				Helper::EntityState newStates;
			};

			std::vector<EntityData> entities;
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
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesCreation& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesDelete& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesStateUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, NetworkStrings& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data);
	}
}

#include <CommonLib/Protocol/Packets.inl>

#endif
