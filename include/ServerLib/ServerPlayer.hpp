// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLAYER_HPP
#define TSOM_SERVERLIB_SERVERPLAYER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/PlayerIndex.hpp>
#include <ServerLib/SessionVisibilityHandler.hpp>
#include <Nazara/Core/HandledObject.hpp>
#include <Nazara/Core/ObjectHandle.hpp>
#include <entt/entt.hpp>
#include <string>

namespace tsom
{
	class CharacterController;
	class NetworkSession;
	class ServerPlayer;
	class ServerInstance;

	using ServerPlayerHandle = Nz::ObjectHandle<ServerPlayer>;

	class TSOM_SERVERLIB_API ServerPlayer : public Nz::HandledObject<ServerPlayer>
	{
		public:
			inline ServerPlayer(ServerInstance& instance, PlayerIndex playerIndex, NetworkSession* session, std::string nickname);
			ServerPlayer(const ServerPlayer&) = delete;
			ServerPlayer(ServerPlayer&&) = delete;
			~ServerPlayer() = default;

			void Destroy();

			inline entt::handle GetControlledEntity() const;
			inline const std::string& GetNickname() const;
			inline PlayerIndex GetPlayerIndex() const;
			inline ServerInstance& GetServerInstance();
			inline const ServerInstance& GetServerInstance() const;
			inline NetworkSession* GetSession();
			inline const NetworkSession* GetSession() const;
			inline SessionVisibilityHandler& GetVisibilityHandler();
			inline const SessionVisibilityHandler& GetVisibilityHandler() const;

			void HandleInputs(const PlayerInputs& inputs);

			void Respawn();

			ServerPlayer& operator=(const ServerPlayer&) = delete;
			ServerPlayer& operator=(ServerPlayer&&) = delete;

		private:
			PlayerIndex m_playerIndex;
			std::shared_ptr<CharacterController> m_controller;
			std::string m_nickname;
			entt::handle m_controlledEntity;
			NetworkSession* m_session;
			SessionVisibilityHandler m_visibilityHandler;
			ServerInstance& m_instance;
	};
}

#include <ServerLib/ServerPlayer.inl>

#endif // TSOM_SERVERLIB_SERVERPLAYER_HPP
