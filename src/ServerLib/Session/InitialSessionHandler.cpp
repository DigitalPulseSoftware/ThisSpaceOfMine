// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <ServerLib/NetworkedEntitiesSystem.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/Version.hpp>
#include <fmt/color.h>
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
		std::uint32_t majorVersion, minorVersion, patchVersion;
		DecodeVersion(authRequest.gameVersion, majorVersion, minorVersion, patchVersion);

		fmt::print("Auth request from {0} (version {1}.{2}.{3})\n", static_cast<std::string_view>(authRequest.nickname), majorVersion, minorVersion, patchVersion);

		auto FailAuth = [&](AuthError err)
		{
			Packets::AuthResponse response;
			response.authResult = Nz::Err(err);

			GetSession()->SendPacket(response);

			GetSession()->Disconnect(DisconnectionType::Later);
		};

		if (authRequest.gameVersion < Constants::ProtocolRequiredClientVersion)
		{
			fmt::print(fg(fmt::color::red), "{0} authentication failed (version is too old)\n", static_cast<std::string_view>(authRequest.nickname));
			return FailAuth(AuthError::UpgradeRequired);
		}

		if (authRequest.gameVersion > GameVersion)
		{
			fmt::print(fg(fmt::color::red), "{0} authentication failed (version is more recent than server's)\n", static_cast<std::string_view>(authRequest.nickname));
			return FailAuth(AuthError::ServerIsOutdated);
		}

		fmt::print("{0} authenticated\n", static_cast<std::string_view>(authRequest.nickname));

		ServerPlayer* player = m_instance.CreatePlayer(GetSession(), std::move(authRequest.nickname));

		Packets::AuthResponse response;
		response.authResult = Nz::Ok();
		response.ownPlayerIndex = player->GetPlayerIndex();

		GetSession()->SendPacket(response);

		auto& networkedEntities = m_instance.GetWorld().GetSystem<NetworkedEntitiesSystem>();
		networkedEntities.CreateAllEntities(player->GetVisibilityHandler());

		GetSession()->SetupHandler<PlayerSessionHandler>(player);

		player->Respawn();
	}
}
