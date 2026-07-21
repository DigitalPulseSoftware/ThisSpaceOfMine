// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
TSOM_NETWORK_PACKET(C_AuthRequest)
TSOM_NETWORK_PACKET(S_AuthResponse)

// Debug opcode
TSOM_NETWORK_PACKET(S_DebugDrawLineList)

// Client opcodes
TSOM_NETWORK_PACKET(C_ConnectEntities)
TSOM_NETWORK_PACKET(C_ExitShipControl)
TSOM_NETWORK_PACKET(C_GrabEntity)
TSOM_NETWORK_PACKET(C_Interact)
TSOM_NETWORK_PACKET(C_MineBlock)
TSOM_NETWORK_PACKET(C_PlaceBlock)
TSOM_NETWORK_PACKET(C_PlaceEntity)
TSOM_NETWORK_PACKET(C_RemoveEntity)
TSOM_NETWORK_PACKET(C_SendChatMessage)
TSOM_NETWORK_PACKET(C_SendConsoleCommand)
TSOM_NETWORK_PACKET(C_UpdatePlayerInputs)

// Server opcodes
TSOM_NETWORK_PACKET(S_ChatMessage)
TSOM_NETWORK_PACKET(S_ChunkCreate)
TSOM_NETWORK_PACKET(S_ChunkDestroy)
TSOM_NETWORK_PACKET(S_ChunkReset)
TSOM_NETWORK_PACKET(S_ChunkUpdate)
TSOM_NETWORK_PACKET(S_ConsoleOutput)
TSOM_NETWORK_PACKET(S_EntitiesCreation)
TSOM_NETWORK_PACKET(S_EntitiesDelete)
TSOM_NETWORK_PACKET(S_EntitiesStateUpdate)
TSOM_NETWORK_PACKET(S_EntityEnvironmentUpdate)
TSOM_NETWORK_PACKET(S_EntityProcedureCall)
TSOM_NETWORK_PACKET(S_EntityPropertiesUpdate)
TSOM_NETWORK_PACKET(S_EnvironmentCreate)
TSOM_NETWORK_PACKET(S_EnvironmentDestroy)
TSOM_NETWORK_PACKET(S_EnvironmentsUpdateOwner)
TSOM_NETWORK_PACKET(S_GameData)
TSOM_NETWORK_PACKET(S_NetworkStrings)
TSOM_NETWORK_PACKET(S_PilotShip)
TSOM_NETWORK_PACKET(S_PilotShipFinish)
TSOM_NETWORK_PACKET(S_PlayerJoin)
TSOM_NETWORK_PACKET(S_PlayerLeave)
TSOM_NETWORK_PACKET_LAST(S_PlayerNameUpdate)

#undef TSOM_NETWORK_PACKET
#undef TSOM_NETWORK_PACKET_LAST
