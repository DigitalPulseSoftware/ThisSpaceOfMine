// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP
#define TSOM_CLIENTLIB_CLIENTSESSIONHANDLER_HPP

#include <NazaraUtils/Signal.hpp>
#include <ClientLib/Export.hpp>
#include <CommonLib/SessionHandler.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>

namespace Nz
{
	class EnttWorld;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientSessionHandler : public SessionHandler
	{
		public:
			ClientSessionHandler(NetworkSession* session, Nz::EnttWorld& world);
			~ClientSessionHandler() = default;

			inline entt::handle GetControlledEntity() const;

			void HandlePacket(Packets::AuthResponse&& authResponse);
			void HandlePacket(Packets::ChatMessage&& chatMessage);
			void HandlePacket(Packets::ChunkCreate&& chunkCreate);
			void HandlePacket(Packets::ChunkDestroy&& chunkDestroy);
			void HandlePacket(Packets::ChunkUpdate&& chunkUpdate);
			void HandlePacket(Packets::EntitiesCreation&& entitiesCreation);
			void HandlePacket(Packets::EntitiesDelete&& entitiesDelete);
			void HandlePacket(Packets::EntitiesStateUpdate&& stateUpdate);
			void HandlePacket(Packets::GameData&& gameData);
			void HandlePacket(Packets::PlayerLeave&& playerLeave);
			void HandlePacket(Packets::PlayerJoin&& playerJoin);

			NazaraSignal(OnChatMessage, const std::string& /*message*/, const std::string& /*senderNickname*/);
			NazaraSignal(OnChunkCreate, const Packets::ChunkCreate& /*chunkCreate*/);
			NazaraSignal(OnChunkDestroy, const Packets::ChunkDestroy& /*chunkDestroy*/);
			NazaraSignal(OnChunkUpdate, const Packets::ChunkUpdate& /*gridUpdate*/);
			NazaraSignal(OnControlledEntityChanged, entt::handle /*newEntity*/);
			NazaraSignal(OnControlledEntityStateUpdate, InputIndex /*lastInputIndex*/, const Packets::EntitiesStateUpdate::ControlledCharacter& /*characterData*/);
			NazaraSignal(OnPlayerLeave, const std::string& /*playerName*/);
			NazaraSignal(OnPlayerJoined, const std::string& /*playerName*/);

			static constexpr Nz::UInt32 InvalidEntity = 0xFFFFFFFF;

		private:
			struct PlayerInfo;

			inline const PlayerInfo* FetchPlayerInfo(PlayerIndex playerIndex) const;
			void SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData);

			struct PlayerInfo
			{
				std::string nickname;
			};

			entt::handle m_playerControlledEntity;
			tsl::hopscotch_map<Nz::UInt32, entt::handle> m_networkIdToEntity;
			std::vector<std::optional<PlayerInfo>> m_players; //< FIXME: Nz::SparseVector
			Nz::EnttWorld& m_world;
			Nz::UInt16 m_lastTickIndex;
			Nz::UInt16 m_ownPlayerIndex;
			InputIndex m_lastInputIndex;
	};
}

#include <ClientLib/ClientSessionHandler.inl>

#endif
