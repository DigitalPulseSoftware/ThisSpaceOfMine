// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

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
#include <vector>

namespace tsom
{
	class CharacterController;
	class NetworkSession;
	class ServerEnvironment;
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

			inline const std::shared_ptr<CharacterController>& GetCharacterController();
			inline entt::handle GetControlledEntity() const;
			inline ServerEnvironment* GetEnvironment();
			inline const ServerEnvironment* GetEnvironment() const;
			inline const std::string& GetNickname() const;
			inline PlayerIndex GetPlayerIndex() const;
			inline ServerInstance& GetServerInstance();
			inline const ServerInstance& GetServerInstance() const;
			inline NetworkSession* GetSession();
			inline const NetworkSession* GetSession() const;
			inline SessionVisibilityHandler& GetVisibilityHandler();
			inline const SessionVisibilityHandler& GetVisibilityHandler() const;

			void PushInputs(const PlayerInputs& inputs);

			void Respawn();

			void Tick();

			void UpdateEnvironment(ServerEnvironment* environment);

			ServerPlayer& operator=(const ServerPlayer&) = delete;
			ServerPlayer& operator=(ServerPlayer&&) = delete;

		private:
			std::shared_ptr<CharacterController> m_controller;
			std::string m_nickname;
			std::vector<PlayerInputs> m_inputQueue;
			entt::handle m_controlledEntity;
			NetworkSession* m_session;
			ServerEnvironment* m_environment;
			SessionVisibilityHandler m_visibilityHandler;
			ServerInstance& m_instance;
			PlayerIndex m_playerIndex;
	};
}

#include <ServerLib/ServerPlayer.inl>

#endif // TSOM_SERVERLIB_SERVERPLAYER_HPP
