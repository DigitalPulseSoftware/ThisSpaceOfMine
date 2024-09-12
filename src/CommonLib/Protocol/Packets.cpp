// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/Packets.hpp>
#include <CommonLib/Version.hpp>
#include <CommonLib/Utility/BinaryCompressor.hpp>
#include <NazaraUtils/TypeTraits.hpp>
#include <lz4.h>
#include <fmt/format.h>

namespace tsom
{
	std::array<std::string_view, PacketCount> PacketNames = {
#define TSOM_NETWORK_PACKET(Name) #Name,
#include <CommonLib/Protocol/PacketList.hpp>
	};

	std::string_view ToString(AuthError authError)
	{
		switch (authError)
		{
			case AuthError::InternalError: return "The server encountered an internal error";
			case AuthError::InvalidToken: return "Invalid connection token";
			case AuthError::ProtocolError: return "A protocol error occurred";
			case AuthError::ServerIsOutdated: return "Server is outdated";
			case AuthError::UpgradeRequired: return "Game version upgrade required";
		}

		return "<unknown authentication error>";
	}

	namespace Packets
	{
		namespace Helper
		{
			void Serialize(PacketSerializer& serializer, EntityState& data)
			{
				serializer &= data.position;
				serializer &= data.rotation;
			}

			void Serialize(PacketSerializer& serializer, EnvironmentTransform& data)
			{
				serializer &= data.translation;
				serializer &= data.rotation;
			}

			void Serialize(PacketSerializer& serializer, PlayerControlledData& data)
			{
				serializer &= data.controllingPlayerId;
			}

			void Serialize(PacketSerializer& serializer, PlayerInputs& data)
			{
				serializer &= data.index;

				serializer &= data.jump;
				serializer &= data.moveBackward;
				serializer &= data.moveForward;
				serializer &= data.moveLeft;
				serializer &= data.moveRight;
				serializer &= data.sprint;
				serializer &= data.crouch;

				serializer &= data.pitch;
				serializer &= data.yaw;
			}

			void Serialize(PacketSerializer& serializer, VoxelLocation& data)
			{
				serializer &= data.x;
				serializer &= data.y;
				serializer &= data.z;
			}
		}

		void Serialize(PacketSerializer& serializer, AuthRequest& data)
		{
			serializer &= data.gameVersion;
			if (data.gameVersion < BuildVersion(0, 4, 0)) //< can't use serializer.GetProtocolVersion() as its initialized after this packet
			{
				if (serializer.IsWriting())
				{
					auto& playerData = std::get<AuthRequest::AnonymousPlayerData>(data.token);
					serializer &= playerData.nickname;
				}
				else
				{
					auto& playerData = data.token.emplace<AuthRequest::AnonymousPlayerData>();
					serializer &= playerData.nickname;
				}
			}
			else
			{
				serializer.Serialize(data.token, Nz::Overloaded
				{
					[&](AuthRequest::AuthenticatedPlayerData& playerData)
					{
						Serialize(serializer, playerData.connectionToken);
					},
					[&](AuthRequest::AnonymousPlayerData& playerData)
					{
						serializer &= playerData.nickname;
					}
				});
			}
		}

		void Serialize(PacketSerializer& serializer, AuthResponse& data)
		{
			serializer &= data.authResult;
			if (data.authResult.IsOk())
			{
				serializer &= data.ownPlayerIndex;
			}
		}

		void Serialize(PacketSerializer& serializer, ChatMessage& data)
		{
			serializer &= data.message;

			serializer.SerializePresence(data.playerIndex);
			serializer.Serialize(data.playerIndex);
		}

		void Serialize(PacketSerializer& serializer, ChunkCreate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.entityId;
			serializer &= data.chunkId;
			serializer &= data.chunkLocX;
			serializer &= data.chunkLocY;
			serializer &= data.chunkLocZ;
			serializer &= data.chunkSizeX;
			serializer &= data.chunkSizeY;
			serializer &= data.chunkSizeZ;
			serializer &= data.cellSize;
		}

		void Serialize(PacketSerializer& serializer, ChunkDestroy& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.entityId;
			serializer &= data.chunkId;
		}

		void Serialize(PacketSerializer& serializer, ChunkReset& data)
		{
			// FIXME: Handle endianness

			serializer &= data.tickIndex;
			serializer &= data.entityId;
			serializer &= data.chunkId;

			serializer.SerializeArraySize(data.content);
			std::size_t bufferSize = data.content.size() * sizeof(BlockIndex);

			BinaryCompressor& binaryCompressor = serializer.GetBinaryCompressor();
			if (serializer.IsWriting())
			{
				std::optional compressedData = binaryCompressor.Compress(data.content.data(), bufferSize);
				if (!compressedData)
					throw std::runtime_error("failed to compress chunk");

				std::span<Nz::UInt8>& buffer = *compressedData;

				CompressedUnsigned<Nz::UInt32> compressedSize(Nz::SafeCast<Nz::UInt32>(buffer.size()));
				serializer &= compressedSize;

				serializer.Write(buffer.data(), buffer.size());
			}
			else
			{
				CompressedUnsigned<Nz::UInt32> compressedSize;
				serializer &= compressedSize;

				Nz::Stream* stream = serializer.GetByteStream().GetStream();
				const char* srcData = static_cast<const char*>(stream->GetMappedPointer()) + stream->GetCursorPos();

				std::optional<std::size_t> decompressedSize = binaryCompressor.Decompress(srcData, compressedSize, data.content.data(), bufferSize);
				if (!decompressedSize)
					throw std::runtime_error("failed to decompress chunk");

				if (*decompressedSize != bufferSize)
					throw std::runtime_error(fmt::format("malformed packet (decompressed size exceeds max packet size: {0} != {1})", *decompressedSize, bufferSize));
			}
		}

		void Serialize(PacketSerializer& serializer, ChunkUpdate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.entityId;
			serializer &= data.chunkId;

			serializer.SerializeArraySize(data.updates);
			for (auto& update : data.updates)
			{
				Helper::Serialize(serializer, update.voxelLoc);
				serializer &= update.newContent;
			}
		}

		void Serialize(PacketSerializer& serializer, DebugDrawLineList& data)
		{
			serializer &= data.environmentId;
			serializer &= data.color;
			serializer &= data.duration;
			serializer &= data.rotation;
			serializer &= data.position;
			serializer &= data.indices;
			serializer &= data.vertices;
		}

		void Serialize(PacketSerializer& serializer, EntitiesCreation& data)
		{
			serializer &= data.tickIndex;

			serializer.SerializeArraySize(data.entities);
			for (auto& entity : data.entities)
			{
				serializer &= entity.environmentId;
				serializer &= entity.entityId;
				Helper::Serialize(serializer, entity.initialStates);

				serializer.SerializePresence(entity.playerControlled);

				serializer.Serialize(entity.entityClass);

				if (entity.playerControlled)
					Helper::Serialize(serializer, *entity.playerControlled);

				serializer.SerializeArraySize(entity.properties);
				for (auto& property : entity.properties)
					serializer &= property;
			}
		}

		void Serialize(PacketSerializer& serializer, EntitiesDelete& data)
		{
			serializer &= data.tickIndex;

			serializer.SerializeArraySize(data.entities);
			for (auto& entityId : data.entities)
				serializer &= entityId;
		}

		void Serialize(PacketSerializer& serializer, EntitiesStateUpdate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.lastInputIndex;

			serializer.SerializePresence(data.controlledCharacter);

			serializer.SerializeArraySize(data.entities);
			for (auto& entity : data.entities)
			{
				serializer &= entity.entityId;
				Helper::Serialize(serializer, entity.newStates);
			}

			if (data.controlledCharacter.has_value())
			{
				serializer &= data.controlledCharacter->position;
				serializer &= data.controlledCharacter->referenceRotation;
				serializer &= data.controlledCharacter->cameraPitch;
				serializer &= data.controlledCharacter->cameraYaw;
			}
		}

		void Serialize(PacketSerializer& serializer, EntityEnvironmentUpdate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.entity;
			serializer &= data.newEnvironmentId;
		}

		TSOM_COMMONLIB_API void Serialize(PacketSerializer& serializer, EntityPropertyUpdate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.entity;
			serializer &= data.propertyIndex;
			serializer &= data.propertyValue;
		}

		void Serialize(PacketSerializer& serializer, EnvironmentCreate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.id;
			Helper::Serialize(serializer, data.transform);
		}

		void Serialize(PacketSerializer& serializer, EnvironmentDestroy& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.id;
		}

		void Serialize(PacketSerializer& serializer, EnvironmentUpdate& data)
		{
			serializer &= data.tickIndex;
			serializer &= data.id;
			Helper::Serialize(serializer, data.transform);
		}

		void Serialize(PacketSerializer& serializer, GameData& data)
		{
			serializer &= data.tickIndex;

			serializer.SerializeArraySize(data.players);
			for (auto& player : data.players)
			{
				serializer &= player.index;
				serializer &= player.nickname;
				serializer &= player.isAuthenticated;
			}
		}

		void Serialize(PacketSerializer& serializer, Interact& data)
		{
			serializer &= data.entityId;
		}

		void Serialize(PacketSerializer& serializer, MineBlock& data)
		{
			serializer &= data.chunkId;
			Helper::Serialize(serializer, data.voxelLoc);
		}

		void Serialize(PacketSerializer& serializer, NetworkStrings& data)
		{
			serializer &= data.startId;

			serializer.SerializeArraySize(data.strings);
			for (auto& string : data.strings)
				serializer &= string;
		}

		void Serialize(PacketSerializer& serializer, PlaceBlock& data)
		{
			serializer &= data.chunkId;
			Helper::Serialize(serializer, data.voxelLoc);
			serializer &= data.newContent;
		}

		void Serialize(PacketSerializer& serializer, PlayerLeave& data)
		{
			serializer &= data.index;
		}

		void Serialize(PacketSerializer& serializer, PlayerJoin& data)
		{
			serializer &= data.index;
			serializer &= data.nickname;
			serializer &= data.isAuthenticated;
		}

		void Serialize(PacketSerializer& serializer, PlayerNameUpdate& data)
		{
			serializer &= data.index;
			serializer &= data.newNickname;
		}

		void Serialize(PacketSerializer& serializer, SendChatMessage& data)
		{
			serializer &= data.message;
		}

		void Serialize(PacketSerializer& serializer, UpdateRootEnvironment& data)
		{
			serializer &= data.newRootEnv;
		}

		void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data)
		{
			Helper::Serialize(serializer, data.inputs);
		}
	}
}
