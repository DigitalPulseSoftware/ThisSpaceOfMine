// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERINSTANCE_HPP
#define TSOM_SERVERLIB_SERVERINSTANCE_HPP

#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <Nazara/Core/Clock.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/MemoryPool.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <memory>
#include <unordered_set>
#include <vector>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class ServerPlanetEnvironment;
	class ServerShipEnvironment;

	class TSOM_SERVERLIB_API ServerInstance
	{
		public:
			struct Config;

			ServerInstance(Nz::ApplicationBase& application, Config config);
			ServerInstance(const ServerInstance&) = delete;
			ServerInstance(ServerInstance&&) = delete;
			~ServerInstance();

			template<typename... Args> NetworkSessionManager& AddSessionManager(Args&&... args);

			void BroadcastChatMessage(std::string message, std::optional<PlayerIndex> senderIndex);

			ServerPlayer* CreatePlayer(NetworkSession* session, std::string nickname);
			ServerShipEnvironment* CreateShip();
			void DestroyPlayer(PlayerIndex playerIndex);
			void DestroyShip(ServerShipEnvironment* ship);

			template<typename F> void ForEachPlayer(F&& functor);
			template<typename F> void ForEachPlayer(F&& functor) const;

			inline Nz::ApplicationBase& GetApplication();
			inline const BlockLibrary& GetBlockLibrary() const;
			inline Nz::Time GetTickDuration() const;

			Nz::Time Update(Nz::Time elapsedTime);

			ServerInstance& operator=(const ServerInstance&) = delete;
			ServerInstance& operator=(ServerInstance&&) = delete;

			struct Config
			{
				std::filesystem::path saveDirectory = Nz::Utf8Path("save/chunks");
				Nz::Time saveInterval = Nz::Time::Seconds(30);
				Nz::UInt32 planetSeed = 42;
				Nz::Vector3ui planetChunkCount = Nz::Vector3ui(5);
				bool pauseWhenEmpty = true;
			};

		private:
			void OnNetworkTick();
			void OnSave();
			void OnTick(Nz::Time elapsedTime);

			std::filesystem::path m_saveDirectory;
			std::unique_ptr<ServerPlanetEnvironment> m_planetEnvironment;
			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
			std::vector<std::unique_ptr<ServerShipEnvironment>> m_shipEnvironments;
			Nz::Bitset<> m_disconnectedPlayers;
			Nz::Bitset<> m_newPlayers;
			Nz::MemoryPool<ServerPlayer> m_players;
			Nz::MillisecondClock m_saveClock;
			Nz::Time m_saveInterval;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
			Nz::UInt16 m_tickIndex;
			Nz::ApplicationBase& m_application;
			BlockLibrary m_blockLibrary;
			bool m_pauseWhenEmpty;
	};
}

#include <ServerLib/ServerInstance.inl>

#endif // TSOM_SERVERLIB_SERVERINSTANCE_HPP
