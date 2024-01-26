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
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PlayerIndex.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Protocol/CompressedInteger.hpp>
#include <CommonLib/Protocol/PacketSerializer.hpp>
#include <CommonLib/Protocol/SecuredString.hpp>

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

	TSOM_COMMONLIB_API extern std::array<std::string_view, PacketCount> PacketNames;

	namespace Packets
	{
		namespace Helper
		{
			using ChunkId = Nz::UInt16;
			using EntityId = Nz::UInt16;

			struct EntityState
			{
				Nz::Quaternionf rotation;
				Nz::Vector3f position;
			};

			struct PlayerControlledData
			{
				PlayerIndex controllingPlayerId;
			};

			struct VoxelLocation
			{
				Nz::UInt8 x;
				Nz::UInt8 y;
				Nz::UInt8 z;
			};

			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityState& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerControlledData& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerInputs& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, VoxelLocation& data);
		}

		struct AuthRequest
		{
			SecuredString<Constants::PlayerMaxNicknameLength> nickname;
		};

		struct AuthResponse
		{
			bool succeeded;
			PlayerIndex ownPlayerIndex;
		};

		struct ChatMessage
		{
			std::optional<PlayerIndex> playerIndex;
			SecuredString<1024> message; //< Don't use Constants::ChatMaxMessageLength to allow larger messages in some cases
		};

		struct ChunkCreate
		{
			Helper::ChunkId chunkId;
			CompressedUnsigned<Nz::UInt32> chunkLocX;
			CompressedUnsigned<Nz::UInt32> chunkLocY;
			CompressedUnsigned<Nz::UInt32> chunkLocZ;
			CompressedUnsigned<Nz::UInt32> chunkSizeX;
			CompressedUnsigned<Nz::UInt32> chunkSizeY;
			CompressedUnsigned<Nz::UInt32> chunkSizeZ;
			float cellSize;
			std::vector<Nz::UInt8> content;
		};

		struct ChunkDestroy
		{
			Helper::ChunkId chunkId;
		};

		struct ChunkUpdate
		{
			Helper::ChunkId chunkId;

			struct BlockUpdate
			{
				Helper::VoxelLocation voxelLoc;
				Nz::UInt8 newContent;
			};

			std::vector<BlockUpdate> updates;
		};

		struct EntitiesCreation
		{
			struct EntityData
			{
				Helper::EntityId entityId;
				Helper::EntityState initialStates;
				std::optional<Helper::PlayerControlledData> playerControlled;
			};

			Nz::UInt16 tickIndex;
			std::vector<EntityData> entities;
		};

		struct EntitiesDelete
		{
			Nz::UInt16 tickIndex;
			std::vector<Helper::EntityId> entities;
		};

		struct EntitiesStateUpdate
		{
			struct ControlledCharacter
			{
				Nz::DegreeAnglef cameraPitch;
				Nz::DegreeAnglef cameraYaw;
				Nz::Quaternionf referenceRotation;
				Nz::Vector3f position;
			};

			struct EntityData
			{
				Helper::EntityId entityId;
				Helper::EntityState newStates;
			};

			Nz::UInt16 tickIndex;
			InputIndex lastInputIndex;
			std::optional<ControlledCharacter> controlledCharacter;
			std::vector<EntityData> entities;
		};

		struct GameData
		{
			struct PlayerData
			{
				PlayerIndex index;
				SecuredString<Constants::PlayerMaxNicknameLength> nickname;
			};

			std::vector<PlayerData> players;
			Nz::UInt16 tickIndex;
		};

		struct MineBlock
		{
			Helper::ChunkId chunkId;
			Helper::VoxelLocation voxelLoc;
		};

		struct NetworkStrings
		{
			CompressedUnsigned<Nz::UInt32> startId;
			std::vector<SecuredString<1024>> strings;
		};

		struct PlaceBlock
		{
			Helper::ChunkId chunkId;
			Helper::VoxelLocation voxelLoc;
			Nz::UInt8 newContent;
		};

		struct PlayerJoin
		{
			PlayerIndex index;
			SecuredString<Constants::PlayerMaxNicknameLength> nickname;
		};

		struct PlayerLeave
		{
			PlayerIndex index;
		};

		struct SendChatMessage
		{
			SecuredString<Constants::ChatMaxMessageLength> message;
		};

		struct UpdatePlayerInputs
		{
			PlayerInputs inputs;
		};

		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, AuthRequest& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, AuthResponse& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ChatMessage& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ChunkCreate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ChunkDestroy& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ChunkUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesCreation& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesDelete& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesStateUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, GameData& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, MineBlock& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, NetworkStrings& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlaceBlock& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerJoin& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerLeave& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, SendChatMessage& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data);
	}
}

#include <CommonLib/Protocol/Packets.inl>

#endif
