// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
#define TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/EnvironmentTransform.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <Nazara/Core/Node.hpp>
#include <NazaraUtils/Signal.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>

namespace Nz
{
	class Animation;
	class ApplicationBase;
	class EnttWorld;
	class Model;
	class Node;
	class TextSprite;
}

namespace tsom
{
	class ClientBlockLibrary;
	class GravityController;
	struct PlayerAnimationAssets;

	class TSOM_CLIENTLIB_API ClientSessionHandler : public SessionHandler
	{
		public:
			struct PlayerInfo;

			ClientSessionHandler(NetworkSession* session, Nz::ApplicationBase& app, Nz::EnttWorld& world, ClientBlockLibrary& blockLibrary);
			~ClientSessionHandler() = default;

			inline PlayerInfo* FetchPlayerInfo(PlayerIndex playerIndex);
			inline const PlayerInfo* FetchPlayerInfo(PlayerIndex playerIndex) const;
			inline Nz::UInt16 GetLocalPlayerIndex() const;

			void HandlePacket(Packets::S_AuthResponse&& authResponse);
			void HandlePacket(Packets::S_ChatMessage&& chatMessage);
			void HandlePacket(Packets::S_ChunkCreate&& chunkCreate);
			void HandlePacket(Packets::S_ChunkDestroy&& chunkDestroy);
			void HandlePacket(Packets::S_ChunkReset&& chunkReset);
			void HandlePacket(Packets::S_ChunkUpdate&& chunkUpdate);
			void HandlePacket(Packets::S_ConsoleOutput&& consoleOutput);
			void HandlePacket(Packets::S_DebugDrawLineList&& debugDrawLineList);
			void HandlePacket(Packets::S_EntitiesCreation&& entitiesCreation);
			void HandlePacket(Packets::S_EntitiesDelete&& entitiesDelete);
			void HandlePacket(Packets::S_EntitiesStateUpdate&& stateUpdate);
			void HandlePacket(Packets::S_EntityEnvironmentUpdate&& environmentUpdate);
			void HandlePacket(Packets::S_EntityProcedureCall&& procedureCall);
			void HandlePacket(Packets::S_EntityPropertiesUpdate&& propertyUpdate);
			void HandlePacket(Packets::S_EnvironmentCreate&& envCreate);
			void HandlePacket(Packets::S_EnvironmentDestroy&& envDestroy);
			void HandlePacket(Packets::S_EnvironmentsUpdateOwner&& envOwnerUpdate);
			void HandlePacket(Packets::S_GameData&& gameData);
			void HandlePacket(Packets::S_NetworkStrings&& networkStrings);
			void HandlePacket(Packets::S_PilotShip&& pilotShip);
			void HandlePacket(Packets::S_PilotShipFinish&& pilotShipFinish);
			void HandlePacket(Packets::S_PlanetEnvironmentRotation&& planetEnvironmentRotation);
			void HandlePacket(Packets::S_PlayerJoin&& playerJoin);
			void HandlePacket(Packets::S_PlayerLeave&& playerLeave);
			void HandlePacket(Packets::S_PlayerNameUpdate&& playerNameUpdate);

			NazaraSignal(OnAuthResponse, Packets::S_AuthResponse& /*authResponse*/);
			NazaraSignal(OnChatMessage, const std::string& /*message*/);
			NazaraSignal(OnChunkCreate, Packets::S_ChunkCreate& /*chunkCreate*/);
			NazaraSignal(OnChunkDestroy, Packets::S_ChunkDestroy& /*chunkDestroy*/);
			NazaraSignal(OnChunkReset, Packets::S_ChunkReset& /*chunkReset*/);
			NazaraSignal(OnChunkUpdate, Packets::S_ChunkUpdate& /*chunkUpdate*/);
			NazaraSignal(OnConsoleOutput, const Nz::Color& /*color*/, std::string_view /*message*/);
			NazaraSignal(OnDebugDrawLineList, Packets::S_DebugDrawLineList& /*debugDrawLineList*/);
			NazaraSignal(OnEntitiesCreation, Packets::S_EntitiesCreation& /*entitiesCreation*/);
			NazaraSignal(OnEntitiesDelete, Packets::S_EntitiesDelete& /*entitiesDelete*/);
			NazaraSignal(OnEntitiesStateUpdate, Packets::S_EntitiesStateUpdate& /*stateUpdate*/);
			NazaraSignal(OnEntityEnvironmentUpdate, Packets::S_EntityEnvironmentUpdate& /*environmentUpdate*/);
			NazaraSignal(OnEntityProcedureCall, Packets::S_EntityProcedureCall& /*procedureCall*/);
			NazaraSignal(OnEntityPropertiesUpdate, Packets::S_EntityPropertiesUpdate& /*propertyUpdate*/);
			NazaraSignal(OnEnvironmentCreate, Packets::S_EnvironmentCreate& /*envCreate*/);
			NazaraSignal(OnEnvironmentDestroy, Packets::S_EnvironmentDestroy& /*envDestroy*/);
			NazaraSignal(OnEnvironmentsUpdateOwner, Packets::S_EnvironmentsUpdateOwner& /*envOwnerUpdate*/);
			NazaraSignal(OnGameData, Packets::S_GameData& /*gameData*/);
			NazaraSignal(OnPilotShip, Packets::S_PilotShip& /*planetShip*/);
			NazaraSignal(OnPilotShipFinish, Packets::S_PilotShipFinish& /*planetShipFinish*/);
			NazaraSignal(OnPlanetEnvironmentRotation, Packets::S_PlanetEnvironmentRotation& /*planetEnvironmentRotation*/);
			NazaraSignal(OnPlayerChatMessage, const std::string& /*message*/, const PlayerInfo& /*playerInfo*/);
			NazaraSignal(OnPlayerJoined, const PlayerInfo& /*playerInfo*/);
			NazaraSignal(OnPlayerLeave, const PlayerInfo& /*playerInfo*/);
			NazaraSignal(OnPlayerNameUpdate, const PlayerInfo& /*playerInfo*/, const std::string& /*newNickname*/);

			struct PlayerInfo
			{
				std::string nickname;
				bool isAuthenticated;
				std::shared_ptr<Nz::TextSprite> textSprite;
			};

		private:
			std::vector<std::optional<PlayerInfo>> m_players; //< FIXME: Nz::SparseVector
			Nz::ApplicationBase& m_app;
			Nz::UInt16 m_localPlayerIndex;
	};
}

#include <ClientLib/ClientSessionHandler.inl>

#endif // TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
