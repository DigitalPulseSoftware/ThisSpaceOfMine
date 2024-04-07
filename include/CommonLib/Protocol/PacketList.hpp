// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

// no header guards

#ifndef TSOM_NETWORK_PACKET
#error TSOM_NETWORK_PACKET must defined before including this file
#endif

#ifndef TSOM_NETWORK_PACKET_LAST
#define TSOM_NETWORK_PACKET_LAST(Name) TSOM_NETWORK_PACKET(Name)
#endif

// Keep these two in order to keep their opcode stable (as they're responsible for protocol version check)
TSOM_NETWORK_PACKET(AuthRequest)
TSOM_NETWORK_PACKET(AuthResponse)

TSOM_NETWORK_PACKET(ChatMessage)
TSOM_NETWORK_PACKET(ChunkCreate)
TSOM_NETWORK_PACKET(ChunkDestroy)
TSOM_NETWORK_PACKET(ChunkReset)
TSOM_NETWORK_PACKET(ChunkUpdate)
TSOM_NETWORK_PACKET(EntitiesCreation)
TSOM_NETWORK_PACKET(EntitiesDelete)
TSOM_NETWORK_PACKET(EntitiesStateUpdate)
TSOM_NETWORK_PACKET(EntityEnvironmentUpdate)
TSOM_NETWORK_PACKET(EnvironmentCreate)
TSOM_NETWORK_PACKET(EnvironmentDestroy)
TSOM_NETWORK_PACKET(EnvironmentUpdate)
TSOM_NETWORK_PACKET(GameData)
TSOM_NETWORK_PACKET(MineBlock)
TSOM_NETWORK_PACKET(NetworkStrings)
TSOM_NETWORK_PACKET(PlaceBlock)
TSOM_NETWORK_PACKET(PlayerLeave)
TSOM_NETWORK_PACKET(PlayerJoin)
TSOM_NETWORK_PACKET(PlayerNameUpdate)
TSOM_NETWORK_PACKET(SendChatMessage)
TSOM_NETWORK_PACKET(UpdateRootEnvironment)
TSOM_NETWORK_PACKET_LAST(UpdatePlayerInputs)

#undef TSOM_NETWORK_PACKET
#undef TSOM_NETWORK_PACKET_LAST
