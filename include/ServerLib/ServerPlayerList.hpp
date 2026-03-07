// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLAYERLIST_HPP
#define TSOM_SERVERLIB_SERVERPLAYERLIST_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/MovablePtr.hpp>

namespace tsom
{
	class ServerPlayer;

	class TSOM_SERVERLIB_API ServerPlayerList
	{
		public:
			inline ServerPlayerList(ServerInstance& serverInstance);
			ServerPlayerList(const ServerPlayerList&) = default;
			ServerPlayerList(ServerPlayerList&&) = default;
			~ServerPlayerList() = default;

			template<typename F> void ForEachPlayer(F&& callback);
			template<typename F> void ForEachPlayer(F&& callback) const;

			bool IsPlayerRegistered(ServerPlayer* player) const;

			void RegisterPlayer(ServerPlayer* player);
			void UnregisterPlayer(ServerPlayer* player);

			ServerPlayerList& operator=(const ServerPlayerList&) = default;
			ServerPlayerList& operator=(ServerPlayerList&&) = default;

		private:
			Nz::Bitset<Nz::UInt64> m_registeredPlayers;
			Nz::MovablePtr<ServerInstance> m_serverInstance;
	};
}

#include <ServerLib/ServerPlayerList.inl>

#endif // TSOM_SERVERLIB_SERVERPLAYERLIST_HPP
