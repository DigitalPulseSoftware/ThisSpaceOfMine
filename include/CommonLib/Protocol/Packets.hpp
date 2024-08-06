// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_PACKETS_HPP
#define TSOM_COMMONLIB_PROTOCOL_PACKETS_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/EnvironmentTransform.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PlayerIndex.hpp>
#include <CommonLib/PlayerInputs.hpp>
#include <CommonLib/Protocol/CompressedInteger.hpp>
#include <CommonLib/Protocol/ConnectionToken.hpp>
#include <CommonLib/Protocol/PacketSerializer.hpp>
#include <CommonLib/Protocol/SecuredString.hpp>
#include <Nazara/Math/Quaternion.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/Result.hpp>
#include <NazaraUtils/TypeList.hpp>

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

	enum class AuthError : Nz::UInt8
	{
		InvalidToken = 3,
		ProtocolError = 2,
		ServerIsOutdated = 0,
		UpgradeRequired = 1,
	};

	TSOM_COMMONLIB_API std::string_view ToString(AuthError authError);

	namespace Packets
	{
		namespace Helper
		{
			using ChunkId = Nz::UInt16;
			using EntityId = Nz::UInt16;
			using EnvironmentId = Nz::UInt8;

			struct EntityState
			{
				Nz::Quaternionf rotation;
				Nz::Vector3f position;
			};

			struct PlanetData
			{
				float cellSize;
				float cornerRadius;
				float gravity;
			};

			struct PlayerControlledData
			{
				PlayerIndex controllingPlayerId;
			};

			struct ShipData
			{
				float cellSize;
			};

			struct VoxelLocation
			{
				Nz::UInt8 x;
				Nz::UInt8 y;
				Nz::UInt8 z;
			};

			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityState& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EnvironmentTransform& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerControlledData& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerInputs& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, VoxelLocation& data);
		}

		struct AuthRequest
		{
			Nz::UInt32 gameVersion;

			struct AuthenticatedPlayerData
			{
				ConnectionToken connectionToken;
			};

			struct AnonymousPlayerData
			{
				SecuredString<Constants::PlayerMaxNicknameLength> nickname;
			};

			std::variant<AuthenticatedPlayerData, AnonymousPlayerData> token;
		};

		struct AuthResponse
		{
			Nz::Result<void, AuthError> authResult = Nz::Err(AuthError::UpgradeRequired); // To allow type to be default constructed

			// Only present if authentication succeeded
			PlayerIndex ownPlayerIndex;
		};

		struct ChatMessage
		{
			std::optional<PlayerIndex> playerIndex;
			SecuredString<Constants::ChatMaxMessageLength> message;
		};

		struct ChunkCreate
		{
			Nz::UInt16 tickIndex;
			Helper::ChunkId chunkId;
			Helper::EntityId entityId;
			CompressedSigned<Nz::Int32> chunkLocX;
			CompressedSigned<Nz::Int32> chunkLocY;
			CompressedSigned<Nz::Int32> chunkLocZ;
			CompressedUnsigned<Nz::UInt32> chunkSizeX;
			CompressedUnsigned<Nz::UInt32> chunkSizeY;
			CompressedUnsigned<Nz::UInt32> chunkSizeZ;
			float cellSize;
		};

		struct ChunkDestroy
		{
			Nz::UInt16 tickIndex;
			Helper::ChunkId chunkId;
			Helper::EntityId entityId;
		};

		struct ChunkReset
		{
			Nz::UInt16 tickIndex;
			Helper::ChunkId chunkId;
			Helper::EntityId entityId;
			std::vector<BlockIndex> content;
		};

		struct ChunkUpdate
		{
			struct BlockUpdate
			{
				Helper::VoxelLocation voxelLoc;
				BlockIndex newContent;
			};

			Nz::UInt16 tickIndex;
			Helper::ChunkId chunkId;
			Helper::EntityId entityId;
			std::vector<BlockUpdate> updates;
		};

		struct DebugDrawLineList
		{
			Helper::EnvironmentId environmentId;
			std::vector<Nz::UInt16> indices;
			std::vector<Nz::Vector3f> vertices;
			Nz::Color color;
			Nz::Quaternionf rotation;
			Nz::Vector3f position;
			float duration;
		};

		struct EntitiesCreation
		{
			struct EntityData
			{
				Helper::EnvironmentId environmentId;
				Helper::EntityId entityId;
				Helper::EntityState initialStates;
				std::optional<Helper::PlanetData> planet;
				std::optional<Helper::PlayerControlledData> playerControlled;
				std::optional<Helper::ShipData> ship;
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

		struct EntityEnvironmentUpdate
		{
			Nz::UInt16 tickIndex;
			Helper::EntityId entity;
			Helper::EnvironmentId newEnvironmentId;
		};

		struct EnvironmentCreate
		{
			Nz::UInt16 tickIndex;
			Helper::EnvironmentId id;
			EnvironmentTransform transform;
		};

		struct EnvironmentDestroy
		{
			Nz::UInt16 tickIndex;
			Helper::EnvironmentId id;
		};

		struct EnvironmentUpdate
		{
			Nz::UInt16 tickIndex;
			Helper::EnvironmentId id;
			EnvironmentTransform transform;
		};

		struct GameData
		{
			struct PlayerData
			{
				PlayerIndex index;
				SecuredString<Constants::PlayerMaxNicknameLength * 2> nickname;
				bool isAuthenticated;
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

		struct PlayerLeave
		{
			PlayerIndex index;
		};

		struct PlayerJoin
		{
			PlayerIndex index;
			SecuredString<Constants::PlayerMaxNicknameLength * 2> nickname;
			bool isAuthenticated;
		};

		struct PlayerNameUpdate
		{
			PlayerIndex index;
			SecuredString<Constants::PlayerMaxNicknameLength * 2> newNickname;
		};

		struct SendChatMessage
		{
			SecuredString<Constants::ChatMaxPlayerMessageLength> message;
		};

		struct UpdateRootEnvironment
		{
			Helper::EnvironmentId newRootEnv;
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
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ChunkReset& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, ChunkUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, DebugDrawLineList& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesCreation& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesDelete& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntitiesStateUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityEnvironmentUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EnvironmentCreate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EnvironmentDestroy& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EnvironmentUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, GameData& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, MineBlock& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, NetworkStrings& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlaceBlock& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerLeave& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerJoin& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerNameUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, SendChatMessage& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, UpdateRootEnvironment& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data);
	}
}

#include <CommonLib/Protocol/Packets.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_PACKETS_HPP
