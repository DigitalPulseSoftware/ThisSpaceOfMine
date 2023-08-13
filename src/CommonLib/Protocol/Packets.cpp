// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/Packets.hpp>

namespace tsom
{
	std::array<std::string_view, PacketCount> PacketNames = {
#define TSOM_NETWORK_PACKET(Name) #Name,
#include <CommonLib/Protocol/PacketList.hpp>
	};

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
				serializer &= data.jump;
				serializer &= data.moveBackward;
				serializer &= data.moveForward;
				serializer &= data.moveLeft;
				serializer &= data.moveRight;
				serializer &= data.sprint;

				serializer &= data.orientation;
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
			serializer &= data.nickname;
		}

		void Serialize(PacketSerializer& serializer, AuthResponse& data)
		{
			serializer &= data.succeeded;
			serializer &= data.ownPlayerIndex;
		}

		void Serialize(PacketSerializer& serializer, ChatMessage& data)
		{
			serializer &= data.message;

			bool hasPlayer;
			if (serializer.IsWriting())
				hasPlayer = data.playerIndex.has_value();

			serializer &= hasPlayer;
			if (!serializer.IsWriting() && hasPlayer)
				data.playerIndex.emplace();

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

			serializer.SerializeArraySize(data.content);
			for (Nz::UInt8& block : data.content)
				serializer &= block;
		}

		void Serialize(PacketSerializer& serializer, ChunkDestroy& data)
		{
			serializer &= data.chunkId;
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
			serializer.SerializeArraySize(data.entities);
			for (auto& entity : data.entities)
			{
				serializer &= entity.entityId;
				Helper::Serialize(serializer, entity.initialStates);

				serializer.SerializePresence(entity.playerControlled);

				if (entity.playerControlled)
					Helper::Serialize(serializer, *entity.playerControlled);
			}
		}

		void Serialize(PacketSerializer& serializer, EntitiesDelete& data)
		{
			serializer.SerializeArraySize(data.entities);
			for (auto& entityId : data.entities)
				serializer &= entityId;
		}

		void Serialize(PacketSerializer& serializer, EntitiesStateUpdate& data)
		{
			serializer.SerializeArraySize(data.entities);
			for (auto& entity : data.entities)
			{
				serializer &= entity.entityId;
				Helper::Serialize(serializer, entity.newStates);
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
