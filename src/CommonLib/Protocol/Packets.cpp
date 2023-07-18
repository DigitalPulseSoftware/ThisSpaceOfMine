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
			serializer &= data.position;
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
			serializer &= data.position;
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

		void Serialize(PacketSerializer& serializer, UpdatePlayerInputs& data)
		{
			Helper::Serialize(serializer, data.inputs);
		}

		void Serialize(PacketSerializer& serializer, VoxelGridUpdate& data)
		{
			serializer.SerializeArraySize(data.updates);
			for (auto& update : data.updates)
			{
				serializer &= update.position;
				serializer &= update.newContent;
			}
		}
	}
}
