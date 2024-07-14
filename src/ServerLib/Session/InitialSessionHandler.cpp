// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Version.hpp>
#include <ServerLib/NetworkedEntitiesSystem.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthResponse>, { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } }
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

		auto FailAuth = [&](AuthError err)
		{
			Packets::AuthResponse response;
			response.authResult = Nz::Err(err);

			GetSession()->SendPacket(response);

			GetSession()->Disconnect(DisconnectionType::Later);
		};
	
		std::string login;
		bool success = std::visit(Nz::Overloaded
		{
			[&](Packets::AuthRequest::AnonymousPlayerData& anonymousData) -> bool
			{
				login = std::move(anonymousData.nickname);

				fmt::print("{0} authenticated\n", login);
				return true;
			},
			[&](Packets::AuthRequest::AuthenticatedPlayerData& authenticatedData) -> bool
			{
				const std::array<Nz::UInt8, 32>& key = m_instance.GetConnectionTokenEncryptionKey();

				ConnectionTokenPrivate tokenPrivate;
				ConnectionTokenAuth errorCode = ConnectionTokenPrivate::AuthAndDecrypt(authenticatedData.connectionToken, key, &tokenPrivate);
				if (errorCode != ConnectionTokenAuth::Success)
				{
					fmt::print(fg(fmt::color::red), "{0} token is invalid\n", login);
					FailAuth(AuthError::ProtocolError);
					return false;
				}

				login = std::move(tokenPrivate.player.nickname);
				return true;
			}
		}, authRequest.token);

		if (!success)
			return;

		if (login.empty() || login != Nz::Trim(login, Nz::UnicodeAware{}))
		{
			fmt::print(fg(fmt::color::red), "{0} nickname hasn't been trimmed\n", login);
			return FailAuth(AuthError::ProtocolError);
		}

		fmt::print("Auth request from {0} (version {1}.{2}.{3})\n", login, majorVersion, minorVersion, patchVersion);

		if (authRequest.gameVersion < Constants::ProtocolRequiredClientVersion)
		{
			fmt::print(fg(fmt::color::red), "{0} authentication failed (version is too old)\n", login);
			return FailAuth(AuthError::UpgradeRequired);
		}

		if (authRequest.gameVersion > GameVersion)
		{
			fmt::print(fg(fmt::color::red), "{0} authentication failed (version is more recent than server's)\n", login);
			return FailAuth(AuthError::ServerIsOutdated);
		}

		GetSession()->SetProtocolVersion(authRequest.gameVersion);

		fmt::print("{0} authenticated\n", login);

		ServerPlayer* player = m_instance.CreatePlayer(GetSession(), std::move(login));

		Packets::AuthResponse response;
		response.authResult = Nz::Ok();
		response.ownPlayerIndex = player->GetPlayerIndex();

		GetSession()->SendPacket(response);

		auto& networkedEntities = m_instance.GetWorld().GetSystem<NetworkedEntitiesSystem>();
		networkedEntities.CreateAllEntities(player->GetVisibilityHandler());

		GetSession()->SetupHandler<PlayerSessionHandler>(player);

		player->Respawn();
	}

	void InitialSessionHandler::OnDeserializationError(std::size_t packetIndex)
	{
		if (packetIndex == PacketIndex<Packets::AuthRequest>)
		{
			fmt::print("failed to deserialize Auth packet from peer {0}\n", GetSession()->GetPeerId());

			Packets::AuthResponse response;
			response.authResult = Nz::Err(AuthError::ProtocolError);

			GetSession()->SendPacket(response);

			GetSession()->Disconnect(DisconnectionType::Later);
			return;
		}
		else
		{
			fmt::print("failed to deserialize unexpected packet {1} from peer {0}\n", GetSession()->GetPeerId(), PacketNames[packetIndex]);
			GetSession()->Disconnect(DisconnectionType::Kick);
		}
	}

	void InitialSessionHandler::OnUnexpectedPacket(std::size_t packetIndex)
	{
		fmt::print("received unexpected packet {1} from peer {0}\n", GetSession()->GetPeerId(), PacketNames[packetIndex]);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	void InitialSessionHandler::OnUnknownOpcode(Nz::UInt8 opcode)
	{
		fmt::print("received unknown packet (opcode: {1}) from peer {0}\n", GetSession()->GetPeerId(), +opcode);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}
}
