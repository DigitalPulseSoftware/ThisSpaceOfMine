// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Session/InitialSessionHandler.hpp>
#include <CommonLib/NetworkedEntitiesSystem.hpp>
#include <CommonLib/ServerPlayer.hpp>
#include <CommonLib/ServerInstance.hpp>
#include <CommonLib/Session/PlayerSessionHandler.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthResponse>, { 0, Nz::ENetPacketFlag_Reliable } }
	});

	InitialSessionHandler::InitialSessionHandler(ServerInstance& instance, NetworkSession* session) :
	SessionHandler(session),
	m_instance(instance)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
	}

	void InitialSessionHandler::HandlePacket(Packets::AuthRequest&& authRequest)
	{
		fmt::print("auth request from {}\n", authRequest.nickname);

		ServerPlayer* player = m_instance.CreatePlayer(GetSession(), std::move(authRequest.nickname));

		Packets::AuthResponse response;
		response.succeeded = true;
		response.ownPlayerIndex = player->GetPlayerIndex();

		GetSession()->SendPacket(response);

		auto& networkedEntities = m_instance.GetWorld().GetSystem<NetworkedEntitiesSystem>();
		networkedEntities.CreateAllEntities(player->GetVisibilityHandler());

		GetSession()->SetupHandler<PlayerSessionHandler>(player);

		player->Respawn();
	}
}
