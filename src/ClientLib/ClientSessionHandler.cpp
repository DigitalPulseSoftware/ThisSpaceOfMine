// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Graphics/TextSprite.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::C_AuthRequest>,        { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_ExitShipControl>,    { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_Interact>,           { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_MineBlock>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_PlaceBlock>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_SendChatMessage>,    { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_SendConsoleCommand>, { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_UpdatePlayerInputs>, { .channel = 1, .flags = Nz::ENetPacketFlag_Unreliable } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session, Nz::ApplicationBase& app, Nz::EnttWorld& world, ClientBlockLibrary& blockLibrary) :
	SessionHandler(session),
	m_app(app),
	m_localPlayerIndex(InvalidPlayerIndex)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_AuthResponse&& authResponse)
	{
		if (authResponse.authResult.IsOk())
			m_localPlayerIndex = authResponse.ownPlayerIndex;

		OnAuthResponse(authResponse);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChatMessage&& chatMessage)
	{
		if (chatMessage.playerIndex)
		{
			if (chatMessage.playerIndex >= m_players.size())
			{
				spdlog::error("ChatMessage with unknown player index {}", *chatMessage.playerIndex);
				return;
			}

			OnPlayerChatMessage(chatMessage.message, *m_players[*chatMessage.playerIndex]);
		}
		else
			OnChatMessage(chatMessage.message);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkCreate&& chunkCreate)
	{
		OnChunkCreate(chunkCreate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkDestroy&& chunkDestroy)
	{
		OnChunkDestroy(chunkDestroy);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkReset&& chunkReset)
	{
		OnChunkReset(chunkReset);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkUpdate&& chunkUpdate)
	{
		OnChunkUpdate(chunkUpdate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ConsoleOutput&& consoleOutput)
	{
		OnConsoleOutput(consoleOutput.color, consoleOutput.output);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_DebugDrawLineList&& debugDrawLineList)
	{
		OnDebugDrawLineList(debugDrawLineList);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntitiesCreation&& entitiesCreation)
	{
		OnEntitiesCreation(entitiesCreation);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntitiesDelete&& entitiesDelete)
	{
		OnEntitiesDelete(entitiesDelete);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntitiesStateUpdate&& stateUpdate)
	{
		OnEntitiesStateUpdate(stateUpdate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntityEnvironmentUpdate&& environmentUpdate)
	{
		OnEntityEnvironmentUpdate(environmentUpdate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntityProcedureCall&& procedureCall)
	{
		OnEntityProcedureCall(procedureCall);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntityPropertiesUpdate&& propertyUpdate)
	{
		OnEntityPropertiesUpdate(propertyUpdate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EnvironmentCreate&& envCreate)
	{
		OnEnvironmentCreate(envCreate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EnvironmentDestroy&& envDestroy)
	{
		OnEnvironmentDestroy(envDestroy);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EnvironmentsUpdateOwner&& envOwnerUpdate)
	{
		OnEnvironmentsUpdateOwner(envOwnerUpdate);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_GameData&& gameData)
	{
		OnGameData(gameData);

		for (auto& playerData : gameData.players)
		{
			if (playerData.index >= m_players.size())
				m_players.resize(playerData.index + 1);

			auto& playerInfo = m_players[playerData.index].emplace();
			playerInfo.nickname = std::move(playerData.nickname).Str();
			playerInfo.isAuthenticated = playerData.isAuthenticated;
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::S_NetworkStrings&& networkStrings)
	{
		GetSession()->GetStringStore().FillStore(networkStrings.startId, std::move(networkStrings.strings));
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PilotShip&& pilotShip)
	{
		OnPilotShip(pilotShip);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PilotShipFinish&& pilotShipFinish)
	{
		OnPilotShipFinish(pilotShipFinish);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlanetEnvironmentRotation&& planetEnvironmentRotation)
	{
		OnPlanetEnvironmentRotation(planetEnvironmentRotation);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlayerJoin&& playerJoin)
	{
		if (playerJoin.index >= m_players.size())
			m_players.resize(playerJoin.index + 1);

		auto& playerInfo = m_players[playerJoin.index].emplace();
		playerInfo.nickname = std::move(playerJoin.nickname).Str();
		playerInfo.isAuthenticated = playerJoin.isAuthenticated;

		OnPlayerJoined(playerInfo);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlayerLeave&& playerLeave)
	{
		if (playerLeave.index >= m_players.size() || !m_players[playerLeave.index])
		{
			spdlog::error("PlayerLeave with unknown player index {}", playerLeave.index);
			return;
		}

		OnPlayerLeave(*m_players[playerLeave.index]);

		m_players[playerLeave.index].reset();
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlayerNameUpdate&& playerNameUpdate)
	{
		if (playerNameUpdate.index >= m_players.size() || !m_players[playerNameUpdate.index])
		{
			spdlog::error("PlayerNameUpdate with unknown player index {}", playerNameUpdate.index);
			return;
		}

		auto& playerInfo = *m_players[playerNameUpdate.index];

		OnPlayerNameUpdate(playerInfo, playerNameUpdate.newNickname);
		playerInfo.nickname = std::move(playerNameUpdate.newNickname).Str();
		if (playerInfo.textSprite)
			playerInfo.textSprite->Update(Nz::SimpleTextDrawer::Draw(playerInfo.nickname, 48, Nz::TextStyle_Regular, (playerInfo.isAuthenticated) ? Nz::Color::White() : Nz::Color::Gray()), 0.01f);
	}
}
