// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
#define TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/Skeleton.hpp>
#include <NazaraUtils/Signal.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>

namespace Nz
{
	class Animation;
	class ApplicationBase;
	class EnttWorld;
	class Model;
}

namespace tsom
{
	class ClientBlockLibrary;
	struct PlayerAnimationAssets;

	class TSOM_CLIENTLIB_API ClientSessionHandler : public SessionHandler
	{
		public:
			ClientSessionHandler(NetworkSession* session, Nz::ApplicationBase& app, Nz::EnttWorld& world, ClientBlockLibrary& blockLibrary);
			~ClientSessionHandler();

			inline entt::handle GetControlledEntity() const;

			void HandlePacket(Packets::AuthResponse&& authResponse);
			void HandlePacket(Packets::ChatMessage&& chatMessage);
			void HandlePacket(Packets::ChunkCreate&& chunkCreate);
			void HandlePacket(Packets::ChunkDestroy&& chunkDestroy);
			void HandlePacket(Packets::ChunkReset&& chunkReset);
			void HandlePacket(Packets::ChunkUpdate&& chunkUpdate);
			void HandlePacket(Packets::EntitiesCreation&& entitiesCreation);
			void HandlePacket(Packets::EntitiesDelete&& entitiesDelete);
			void HandlePacket(Packets::EntitiesStateUpdate&& stateUpdate);
			void HandlePacket(Packets::GameData&& gameData);
			void HandlePacket(Packets::PlayerLeave&& playerLeave);
			void HandlePacket(Packets::PlayerJoin&& playerJoin);

			NazaraSignal(OnAuthResponse, const Packets::AuthResponse& /*authResponse*/);
			NazaraSignal(OnChatMessage, const std::string& /*message*/, const std::string& /*senderNickname*/);
			NazaraSignal(OnChunkCreate, const Packets::ChunkCreate& /*chunkCreate*/);
			NazaraSignal(OnChunkDestroy, const Packets::ChunkDestroy& /*chunkDestroy*/);
			NazaraSignal(OnChunkReset, const Packets::ChunkReset& /*chunkReset*/);
			NazaraSignal(OnChunkUpdate, const Packets::ChunkUpdate& /*chunkUpdate*/);
			NazaraSignal(OnControlledEntityChanged, entt::handle /*newEntity*/);
			NazaraSignal(OnControlledEntityStateUpdate, InputIndex /*lastInputIndex*/, const Packets::EntitiesStateUpdate::ControlledCharacter& /*characterData*/);
			NazaraSignal(OnPlayerLeave, const std::string& /*playerName*/);
			NazaraSignal(OnPlayerJoined, const std::string& /*playerName*/);

			static constexpr Nz::UInt32 InvalidEntity = 0xFFFFFFFF;

		private:
			struct PlayerInfo;

			inline const PlayerInfo* FetchPlayerInfo(PlayerIndex playerIndex) const;
			void SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData);
			void SetupEntity(entt::handle entity, Packets::Helper::ShipData&& entityData);

			struct PlayerInfo
			{
				std::string nickname;
			};

			struct PlayerModel
			{
				std::shared_ptr<Nz::Model> model;
			};

			entt::handle m_playerControlledEntity;
			tsl::hopscotch_map<Nz::UInt32, entt::handle> m_networkIdToEntity;
			std::optional<PlayerModel> m_playerModel;
			std::shared_ptr<PlayerAnimationAssets> m_playerAnimAssets;
			std::vector<std::optional<PlayerInfo>> m_players; //< FIXME: Nz::SparseVector
			Nz::ApplicationBase& m_app;
			Nz::EnttWorld& m_world;
			ClientBlockLibrary& m_blockLibrary;
			Nz::UInt16 m_lastTickIndex;
			Nz::UInt16 m_ownPlayerIndex;
			InputIndex m_lastInputIndex;
	};
}

#include <ClientLib/ClientSessionHandler.inl>

#endif // TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
