// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/Packets.hpp>
#include <CommonLib/Version.hpp>
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
			serializer &= data.nickname;
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
			serializer &= data.chunkId;
		}

		void Serialize(PacketSerializer& serializer, ChunkReset& data)
		{
			// FIXME: Handle endianness

			serializer &= data.chunkId;

			serializer.SerializeArraySize(data.content);
			std::size_t bufferSize = data.content.size() * sizeof(BlockIndex);

			if (serializer.IsWriting())
			{
				const char* srcData = reinterpret_cast<const char*>(data.content.data());

				int maxCompressedSize = LZ4_compressBound(Nz::SafeCast<int>(bufferSize));

				std::vector<Nz::UInt8> compressedChunk(maxCompressedSize);
				int compressedSizeInt = LZ4_compress_default(srcData, reinterpret_cast<char*>(compressedChunk.data()), Nz::SafeCast<int>(bufferSize), maxCompressedSize);
				if (compressedSizeInt <= 0)
					throw std::runtime_error("failed to compress chunk");

				Nz::UInt16 compressedSize = Nz::UInt16(compressedSizeInt);
				serializer &= compressedSize;

				serializer.Write(compressedChunk.data(), compressedSize * sizeof(BlockIndex));
			}
			else
			{
				Nz::UInt16 compressedSize;
				serializer &= compressedSize;

				Nz::Stream* stream = serializer.GetByteStream().GetStream();
				const char* srcData = static_cast<const char*>(stream->GetMappedPointer()) + stream->GetCursorPos();

				data.content.resize(data.content.size());
				int decompressedSize = LZ4_decompress_safe(srcData, reinterpret_cast<char*>(data.content.data()), Nz::SafeCast<int>(compressedSize), Nz::SafeCast<int>(bufferSize));
				if (decompressedSize < 0)
					throw std::runtime_error("failed to decompress chunk");

				if (decompressedSize != bufferSize)
					throw std::runtime_error(fmt::format("malformed packet (decompressed size exceeds max packet size: {0} != {1})", decompressedSize, bufferSize));
			}
		}

		void Serialize(PacketSerializer& serializer, ChunkUpdate& data)
		{
			serializer &= data.chunkId;

			serializer.SerializeArraySize(data.updates);
			for (auto& update : data.updates)
			{
				Helper::Serialize(serializer, update.voxelLoc);
				serializer &= update.newContent;
			}
		}

		void Serialize(PacketSerializer& serializer, EntitiesCreation& data)
		{
			serializer &= data.tickIndex;

			serializer.SerializeArraySize(data.entities);
			for (auto& entity : data.entities)
			{
				serializer &= entity.entityId;
				Helper::Serialize(serializer, entity.initialStates);

				serializer.SerializePresence(entity.playerControlled);
				serializer.SerializePresence(entity.ship);

				if (entity.playerControlled)
					Helper::Serialize(serializer, *entity.playerControlled);
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

		void Serialize(PacketSerializer& serializer, GameData& data)
		{
			serializer &= data.tickIndex;

			serializer.SerializeArraySize(data.players);
			for (auto& player : data.players)
			{
				serializer &= player.index;
				serializer &= player.nickname;
			}
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

		void Serialize(PacketSerializer& serializer, PlayerJoin& data)
		{
			serializer &= data.index;
			serializer &= data.nickname;
		}

		void Serialize(PacketSerializer& serializer, PlayerLeave& data)
		{
			serializer &= data.index;
		}

		void Serialize(PacketSerializer& serializer, SendChatMessage& data)
		{
			serializer &= data.message;
		}

		void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data)
		{
			Helper::Serialize(serializer, data.inputs);
		}
	}
}
