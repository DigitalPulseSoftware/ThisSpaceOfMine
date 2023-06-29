// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_SERVERPLAYER_HPP
#define TSOM_COMMONLIB_SERVERPLAYER_HPP

#include <CommonLib/Export.hpp>
#include <entt/entt.hpp>
#include <string>

namespace tsom
{
	class NetworkSession;
	class ServerWorld;

	class TSOM_COMMONLIB_API ServerPlayer
	{
		public:
			inline ServerPlayer(ServerWorld& world, std::size_t playerIndex, NetworkSession* session, std::string nickname);
			ServerPlayer(const ServerPlayer&) = delete;
			ServerPlayer(ServerPlayer&&) = delete;
			~ServerPlayer() = default;

			void Respawn();

			ServerPlayer& operator=(const ServerPlayer&) = delete;
			ServerPlayer& operator=(ServerPlayer&&) = delete;

		private:
			std::size_t m_playerIndex;
			std::string m_nickname;
			entt::handle m_controlledEntity;
			NetworkSession* m_session;
			ServerWorld& m_world;
	};
}

#include <CommonLib/ServerPlayer.inl>

#endif // TSOM_COMMONLIB_SERVERPLAYER_HPP
