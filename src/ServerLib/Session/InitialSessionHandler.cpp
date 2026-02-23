// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Version.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::S_AuthResponse>,   { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_NetworkStrings>, { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } }
	});

	InitialSessionHandler::InitialSessionHandler(ServerInstance& instance, NetworkSession* session) :
	SessionHandler(session),
	m_instance(instance)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);

		auto& stringStore = session->GetStringStore();
		instance.GetEntityRegistry().ForEachClass([&](const std::string& className, const EntityClass& /*entityClass*/)
		{
			stringStore.RegisterString(className);
		});
	}

	void InitialSessionHandler::HandlePacket(Packets::C_AuthRequest&& authRequest)
	{
		std::uint32_t majorVersion, minorVersion, patchVersion;
		DecodeVersion(authRequest.gameVersion, majorVersion, minorVersion, patchVersion);

		auto FailAuth = [&](AuthError err)
		{
			Packets::S_AuthResponse response;
			response.authResult = Nz::Err(err);

			GetSession()->SendPacket(response);

			GetSession()->Disconnect(DisconnectionType::Later);
		};

		std::optional<Nz::Uuid> uuid;
		std::string login;
		PlayerPermissionFlags permissions;
		bool success = std::visit(Nz::Overloaded
		{
			[&](Packets::C_AuthRequest::AnonymousPlayerData& anonymousData) -> bool
			{
				login = std::move(anonymousData.nickname).Str();

				spdlog::info("{0} authenticated (anonymous)", login);
				return true;
			},
			[&](Packets::C_AuthRequest::AuthenticatedPlayerData& authenticatedData) -> bool
			{
				const std::array<Nz::UInt8, 32>& key = m_instance.GetConfig().connectionTokenEncryptionKey;

				ConnectionTokenPrivate tokenPrivate;
				ConnectionTokenAuth errorCode = ConnectionTokenPrivate::AuthAndDecrypt(authenticatedData.connectionToken, key, &tokenPrivate);
				if (errorCode != ConnectionTokenAuth::Success)
				{
					spdlog::error("connection token is invalid", login);
					FailAuth(AuthError::InvalidToken);
					return false;
				}

				uuid = tokenPrivate.player.uuid;
				login = std::move(tokenPrivate.player.nickname);

				for (const std::string& permissionStr : tokenPrivate.player.permissions)
				{
					if (std::optional<PlayerPermission> permissionOpt = PlayerPermissionFromString(permissionStr))
						permissions |= *permissionOpt;
					else
						spdlog::warn("unknown permission \"{}\"", permissionStr);
				}

				// Register player token (FIXME: not really the right place)
				auto& playerTokens = m_instance.GetApplication().GetComponent<PlayerTokenAppComponent>();
				playerTokens.Register(tokenPrivate.player.uuid, tokenPrivate.api.url, tokenPrivate.api.refreshToken);

				spdlog::info("{0} authenticated", login);
				return true;
			}
		}, authRequest.token);

		if (!success)
			return;

		if (login.empty() || login != Nz::Trim(login, Nz::UnicodeAware{}))
		{
			spdlog::error("{0} nickname hasn't been trimmed", login);
			return FailAuth(AuthError::ProtocolError);
		}

		spdlog::info("Auth request from {0} (version {1}.{2}.{3})", login, majorVersion, minorVersion, patchVersion);

		if (authRequest.gameVersion < Constants::ProtocolRequiredClientVersion)
		{
			spdlog::error("{0} authentication failed (version is too old)", login);
			return FailAuth(AuthError::UpgradeRequired);
		}

		// Disallow more recent client than the server (except in dev mode for dev version)
		if (authRequest.gameVersion > GameVersion && (!IsDevVersion() || authRequest.gameVersion != Nz::MaxValue<Nz::UInt32>()))
		{
			spdlog::error("{0} authentication failed (version is more recent than server's)", login);
			return FailAuth(AuthError::ServerIsOutdated);
		}

		NetworkSession* session = GetSession();
		session->SetProtocolVersion(authRequest.gameVersion);

		ServerPlayer* player;
		if (uuid.has_value())
			player = m_instance.CreateAuthenticatedPlayer(session, *uuid, std::move(login), permissions);
		else
			player = m_instance.CreateAnonymousPlayer(session, std::move(login));

		if (!player)
			return FailAuth(AuthError::InternalError);

		Packets::S_AuthResponse response;
		response.authResult = Nz::Ok();
		response.ownPlayerIndex = player->GetPlayerIndex();

		session->SendPacket(response);

		session->SendPacket(session->GetStringStore().BuildPacket());

		session->SetupHandler<PlayerSessionHandler>(player);
	}

	void InitialSessionHandler::OnDeserializationError(std::size_t packetIndex)
	{
		if (packetIndex == PacketIndex<Packets::C_AuthRequest>)
		{
			spdlog::warn("failed to deserialize Auth packet from peer {0}", GetSession()->GetPeerId());

			Packets::S_AuthResponse response;
			response.authResult = Nz::Err(AuthError::ProtocolError);

			GetSession()->SendPacket(response);

			GetSession()->Disconnect(DisconnectionType::Later);
			return;
		}
		else
		{
			spdlog::warn("failed to deserialize unexpected packet {1} from peer {0}", GetSession()->GetPeerId(), PacketNames[packetIndex]);
			GetSession()->Disconnect(DisconnectionType::Kick);
		}
	}

	void InitialSessionHandler::OnUnexpectedPacket(std::size_t packetIndex)
	{
		spdlog::warn("received unexpected packet {1} from peer {0}", GetSession()->GetPeerId(), PacketNames[packetIndex]);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	void InitialSessionHandler::OnUnknownOpcode(Nz::UInt8 opcode)
	{
		spdlog::warn("received unknown packet (opcode: {1}) from peer {0}", GetSession()->GetPeerId(), +opcode);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}
}
