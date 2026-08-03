// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PROTOCOL_PACKETS_HPP
#define TSOM_COMMONLIB_PROTOCOL_PACKETS_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/EntityProperties.hpp>
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
#include <NazaraUtils/FixedVector.hpp>
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
		InternalError    = 4,
		InvalidToken     = 3,
		ProtocolError    = 2,
		ServerIsOutdated = 0,
		UpgradeRequired  = 1,
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

			struct PlayerControlledData
			{
				PlayerIndex controllingPlayerId;
			};

			struct EntityData
			{
				EnvironmentId environmentId;
				EntityId entityId;
				EntityState initialStates;
				std::optional<PlayerControlledData> playerControlled;
				std::vector<EntityProperty> properties;
				CompressedUnsigned<Nz::UInt32> entityClass;
			};

			struct VoxelLocation
			{
				Nz::UInt8 x;
				Nz::UInt8 y;
				Nz::UInt8 z;
			};

			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityData& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityState& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EnvironmentTransform& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerControlledData& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, PlayerInputs& data);
			TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, VoxelLocation& data);
		}

		struct C_AuthRequest
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

		struct C_ConnectEntities
		{
			Helper::EntityId sourceEntityId;
			Helper::EntityId targetEntityId;
			Nz::UInt8 sourceEntityPort;
			Nz::UInt8 targetEntityPort;
		};

		struct C_ExitShipControl
		{
		};

		struct C_GrabEntity
		{
		};

		struct C_Interact
		{
			Helper::EntityId entityId;
		};

		struct C_MineBlock
		{
			Helper::ChunkId chunkId;
			Helper::VoxelLocation voxelLoc;
		};

		struct C_PlaceBlock
		{
			Helper::ChunkId chunkId;
			Helper::VoxelLocation voxelLoc;
			Nz::UInt8 newContent;
		};

		struct C_PlaceEntity
		{
			Helper::ChunkId chunkId;
			Helper::VoxelLocation voxelLoc;
			Direction topFace;
			CompressedUnsigned<Nz::UInt32> entityClass;
			Nz::UInt8 entityRotation;
		};

		struct C_RemoveEntity
		{
			Helper::EntityId entityId;
		};

		struct C_SendChatMessage
		{
			SecuredString<Constants::ChatMaxPlayerMessageLength> message;
		};

		struct C_SendConsoleCommand
		{
			SecuredString<Constants::ConsoleMaxCommandLength> command;
		};

		struct C_UpdatePlayerInputs
		{
			PlayerInputs inputs;
		};


		struct S_AuthResponse
		{
			Nz::Result<void, AuthError> authResult = Nz::Err(AuthError::UpgradeRequired); // To allow type to be default constructed

			// Only present if authentication succeeded
			PlayerIndex ownPlayerIndex;
		};

		struct S_ChatMessage
		{
			std::optional<PlayerIndex> playerIndex;
			SecuredString<Constants::ChatMaxMessageLength> message;
		};

		struct S_ConsoleOutput
		{
			Nz::Color color;
			std::string output;
		};

		struct S_ChunkCreate
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

		struct S_ChunkDestroy
		{
			Nz::UInt16 tickIndex;
			Helper::ChunkId chunkId;
			Helper::EntityId entityId;
		};

		struct S_ChunkReset
		{
			Nz::UInt16 tickIndex;
			Helper::ChunkId chunkId;
			Helper::EntityId entityId;
			std::vector<BlockIndex> content;
		};

		struct S_ChunkUpdate
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

		struct S_DebugDrawLineList
		{
			Nz::UInt64 uniqueHash = 0;
			Helper::EnvironmentId environmentId;
			std::vector<Nz::UInt16> indices;
			std::vector<Nz::Vector3f> vertices;
			Nz::Color color;
			float duration;
		};

		struct S_EntitiesCreation
		{
			Nz::UInt16 tickIndex;
			std::vector<Helper::EntityData> entities;
		};

		struct S_EntitiesDelete
		{
			Nz::UInt16 tickIndex;
			std::vector<Helper::EntityId> entities;
		};

		struct S_EntityDistributionUpdate
		{
			Nz::UInt16 tickIndex;
			Helper::EntityId sourceEntity;
			Helper::EntityId targetEntity;
			Nz::UInt8 sourceEntityPort;
			Nz::UInt8 targetEntityPort;
		};

		struct S_EntitiesStateUpdate
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

		struct S_EntityEnvironmentUpdate
		{
			Nz::UInt16 tickIndex;
			Helper::EntityId entity;
			Helper::EnvironmentId newEnvironmentId;
		};

		struct S_EntityProcedureCall
		{
			Nz::UInt16 tickIndex;
			Helper::EntityId entity;
			CompressedUnsigned<Nz::UInt32> rpcIndex;
		};

		struct S_EntityPropertiesUpdate
		{
			struct Properties
			{
				CompressedUnsigned<Nz::UInt32> index;
				EntityProperty value;
			};

			Nz::UInt16 tickIndex;
			Helper::EntityId entity;
			Nz::HybridVector<Properties, 8> properties;
		};

		struct S_EnvironmentCreate
		{
			Nz::UInt16 tickIndex;
			Helper::EnvironmentId id;
			Helper::EntityId ownerEntity;
			std::vector<Helper::EntityData> entities;
		};

		struct S_EnvironmentDestroy
		{
			Nz::UInt16 tickIndex;
			Helper::EnvironmentId id;
		};

		struct S_EnvironmentsUpdateOwner
		{
			struct OwnerUpdate
			{
				Helper::EnvironmentId environment;
				Helper::EntityId newOwner;
			};

			Nz::UInt16 tickIndex;
			Nz::HybridVector<OwnerUpdate, 3> ownerUpdates;
		};

		struct S_GameData
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

		struct S_NetworkStrings
		{
			CompressedUnsigned<Nz::UInt32> startId;
			std::vector<SecuredString<1024>> strings;
		};

		struct S_PilotShip
		{
			Nz::Quaternionf referenceRotation;
			Helper::EntityId shipEntity;
			Helper::EntityId shipExteriorEntity;
		};

		struct S_PilotShipFinish
		{
		};

		struct S_PlayerLeave
		{
			PlayerIndex index;
		};

		struct S_PlayerJoin
		{
			PlayerIndex index;
			SecuredString<Constants::PlayerMaxNicknameLength * 2> nickname;
			bool isAuthenticated;
		};

		struct S_PlayerNameUpdate
		{
			PlayerIndex index;
			SecuredString<Constants::PlayerMaxNicknameLength * 2> newNickname;
		};

		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_AuthRequest& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_ConnectEntities& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_ExitShipControl& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_GrabEntity& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_Interact& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_MineBlock& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_PlaceBlock& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_PlaceEntity& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_RemoveEntity& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_SendChatMessage& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_SendConsoleCommand& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, C_UpdatePlayerInputs& data);

		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_AuthResponse& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_ChatMessage& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_ChunkCreate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_ChunkDestroy& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_ChunkReset& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_ChunkUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_ConsoleOutput& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_DebugDrawLineList& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntitiesCreation& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntitiesDelete& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntitiesStateUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntityDistributionUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntityEnvironmentUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntityProcedureCall& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EntityPropertiesUpdate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EnvironmentCreate& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EnvironmentDestroy& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_EnvironmentsUpdateOwner& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_GameData& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_NetworkStrings& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_PilotShip& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_PilotShipFinish& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_PlayerJoin& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_PlayerLeave& data);
		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, S_PlayerNameUpdate& data);
	}
}

#include <CommonLib/Protocol/Packets.inl>

#endif // TSOM_COMMONLIB_PROTOCOL_PACKETS_HPP
