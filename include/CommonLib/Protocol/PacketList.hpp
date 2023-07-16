// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef TSOM_NETWORK_PACKET
#error TSOM_NETWORK_PACKET must defined before including this file
#endif

#ifndef TSOM_NETWORK_PACKET_LAST
#define TSOM_NETWORK_PACKET_LAST(Name) TSOM_NETWORK_PACKET(Name)
#endif

TSOM_NETWORK_PACKET(AuthRequest)
TSOM_NETWORK_PACKET(AuthResponse)
TSOM_NETWORK_PACKET(NetworkStrings)
TSOM_NETWORK_PACKET(EntitiesCreation)
TSOM_NETWORK_PACKET(EntitiesDelete)
TSOM_NETWORK_PACKET(EntitiesStateUpdate)
TSOM_NETWORK_PACKET(PlayerLeave)
TSOM_NETWORK_PACKET(PlayerJoin)
TSOM_NETWORK_PACKET_LAST(UpdatePlayerInputs)

#undef TSOM_NETWORK_PACKET
#undef TSOM_NETWORK_PACKET_LAST
