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
			~ClientSessionHandler();

			inline entt::handle GetControlledEntity() const;
			const Nz::Node* GetEnvironmentNode(std::size_t environmentIndex) const;
			inline const GravityController* GetGravityController(std::size_t environmentIndex) const;
			inline ScriptingContext& GetScriptingContext();

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

			void LoadScripts(bool isReloading = false);

			NazaraSignal(OnAuthResponse, const Packets::S_AuthResponse& /*authResponse*/);
			NazaraSignal(OnChatMessage, const std::string& /*message*/);
			NazaraSignal(OnConsoleOutput, const Nz::Color& /*color*/, std::string_view /*message*/);
			NazaraSignal(OnControlledEntityChanged, entt::handle /*newEntity*/);
			NazaraSignal(OnControlledEntityStateUpdate, InputIndex /*lastInputIndex*/, const Packets::S_EntitiesStateUpdate::ControlledCharacter& /*characterData*/);
			NazaraSignal(OnControlledShip, entt::handle /*controlledShip*/, entt::handle /*controlledShipExterior*/, const Nz::Quaternionf& /*referenceRotation*/);
			NazaraSignal(OnControlledShipFinished);
			NazaraSignal(OnDebugDrawLineList, const Packets::S_DebugDrawLineList& /*debugDrawLineList*/);
			NazaraSignal(OnPlanetEnvironmentRotation, const Packets::S_PlanetEnvironmentRotation& /*planetEnvironmentRotation*/);
			NazaraSignal(OnPlayerChatMessage, const std::string& /*message*/, const PlayerInfo& /*playerInfo*/);
			NazaraSignal(OnPlayerJoined, const PlayerInfo& /*playerInfo*/);
			NazaraSignal(OnPlayerLeave, const PlayerInfo& /*playerInfo*/);
			NazaraSignal(OnPlayerNameUpdate, const PlayerInfo& /*playerInfo*/, const std::string& /*newNickname*/);

			static constexpr Packets::Helper::EntityId InvalidEntity = Nz::MaxValue();

			struct PlayerInfo
			{
				std::string nickname;
				bool isAuthenticated;
				std::shared_ptr<Nz::TextSprite> textSprite;
			};

		private:
			inline PlayerInfo* FetchPlayerInfo(PlayerIndex playerIndex);
			inline const PlayerInfo* FetchPlayerInfo(PlayerIndex playerIndex) const;
			void HandleEntityCreation(Packets::Helper::EntityData&& entityData);
			void SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData);

			struct DebugDrawLines
			{
				Nz::UInt64 uniqueHash = 0;
				std::size_t environmentId;
				std::vector<Nz::UInt16> indices;
				std::vector<Nz::Vector3f> vertices;
				Nz::Color color;
				Nz::Quaternionf rotation;
				Nz::Vector3f position;
				float duration;
			};

			struct EnvironmentData
			{
				Nz::Bitset<Nz::UInt64> entities;
				entt::handle rootEntity;
				entt::handle visualRootEntity;
				GravityController* gravityController;
			};

			struct EntityData
			{
				Packets::Helper::EnvironmentId environmentIndex;
				entt::handle entity;
			};

			struct PlayerModel
			{
				std::shared_ptr<Nz::Model> model;
			};

			entt::handle m_playerControlledEntity;
			std::optional<PlayerModel> m_playerModel;
			std::shared_ptr<PlayerAnimationAssets> m_playerAnimAssets;
			std::vector<std::optional<EntityData>> m_entities; //< FIXME: Nz::SparseVector
			std::vector<std::optional<EnvironmentData>> m_environments; //< FIXME: Nz::SparseVector
			std::vector<std::optional<PlayerInfo>> m_players; //< FIXME: Nz::SparseVector
			Nz::ApplicationBase& m_app;
			Nz::EnttWorld& m_world;
			ClientBlockLibrary& m_blockLibrary;
			Nz::UInt16 m_lastTickIndex;
			Nz::UInt16 m_ownPlayerIndex;
			ScriptingContext m_scriptingContext;
			EntityRegistry m_entityRegistry;
			InputIndex m_lastInputIndex;
	};
}

#include <ClientLib/ClientSessionHandler.inl>

#endif // TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
