// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERINSTANCE_HPP
#define TSOM_SERVERLIB_SERVERINSTANCE_HPP

#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/EntityRegistry.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <ServerLib/Database/ServerDatabase.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/TimerManager.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/MemoryPool.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <array>
#include <memory>
#include <unordered_set>
#include <vector>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
}

namespace tsom
{
	class ServerPlanetEnvironment;
	class ServerShipEnvironment;

	class TSOM_SERVERLIB_API ServerInstance
	{
		friend class ServerEnvironment;

		public:
			struct Config;
			struct Spawnpoint;

			ServerInstance(Nz::ApplicationBase& application, Config config);
			ServerInstance(const ServerInstance&) = delete;
			ServerInstance(ServerInstance&&) = delete;
			~ServerInstance();

			template<typename... Args> NetworkSessionManager& AddSessionManager(Args&&... args);

			void BroadcastChatMessage(std::string message, std::optional<PlayerIndex> senderIndex = std::nullopt);

			ServerPlayer* CreateAnonymousPlayer(NetworkSession* session, std::string nickname);
			ServerPlayer* CreateAuthenticatedPlayer(NetworkSession* session, const Nz::Uuid& uuid, std::string nickname, PlayerPermissionFlags permissions);
			void DestroyPlayer(PlayerIndex playerIndex);

			inline ServerPlayer* FindPlayerByNickname(std::string_view nickname);
			inline const ServerPlayer* FindPlayerByNickname(std::string_view nickname) const;
			inline ServerPlayer* FindPlayerByUuid(const Nz::Uuid& uuid);
			inline const ServerPlayer* FindPlayerByUuid(const Nz::Uuid& uuid) const;

			template<typename F> void ForEachPlayer(F&& functor);
			template<typename F> void ForEachPlayer(F&& functor) const;

			inline Nz::ApplicationBase& GetApplication();
			inline const BlockLibrary& GetBlockLibrary() const;
			inline const Config& GetConfig() const;
			inline const Spawnpoint& GetDefaultSpawnpoint() const;
			inline EntityRegistry& GetEntityRegistry();
			inline const EntityRegistry& GetEntityRegistry() const;
			inline ServerPlayer* GetPlayer(PlayerIndex playerIndex);
			inline const ServerPlayer* GetPlayer(PlayerIndex playerIndex) const;
			inline ServerDatabase& GetServerDatabase();
			inline Nz::Time GetTickDuration() const;
			inline Nz::TimerManager& GetTickedTimerManager();

			void LoadFromDatabase();

			std::unique_ptr<Nz::EnttWorld> RegisterEnvironment(ServerEnvironment* environment);

			inline void ScheduleForNextTick(std::function<void()>&& callback);

			inline void SetDefaultSpawnpoint(ServerEnvironment* environment, Nz::Vector3f position, Nz::Quaternionf rotation);

			void UnregisterEnvironment(ServerEnvironment* environment, std::unique_ptr<Nz::EnttWorld>&& world);

			Nz::Time Update(Nz::Time elapsedTime);

			ServerInstance& operator=(const ServerInstance&) = delete;
			ServerInstance& operator=(ServerInstance&&) = delete;

			struct Config
			{
				std::array<std::uint8_t, 32> connectionTokenEncryptionKey;
				std::string databaseFile;
				Nz::Time saveInterval = Nz::Time::Seconds(30);
				bool enableDebugDrawer = false;
				bool pauseWhenEmpty = true;
			};

			struct Spawnpoint
			{
				ServerEnvironment* env = nullptr;
				Nz::Quaternionf rotation;
				Nz::Vector3f position;
			};

		private:
			void LoadScripts(bool isReloading = false);
			void OnNetworkTick();
			void OnSave();
			void OnTick(Nz::Time elapsedTime);

			struct PlayerRename
			{
				PlayerIndex playerIndex;
				std::string newNickname;
			};

			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
			std::vector<PlayerRename> m_pendingPlayerRename;
			std::vector<ServerEnvironment*> m_environments;
			std::vector<std::unique_ptr<Nz::EnttWorld>> m_envWorldPool;
			std::vector<std::function<void()>> m_scheduledTickFunctions;
			std::vector<std::function<void()>> m_nextScheduledTickFunctions;
			tsl::hopscotch_map<Nz::UInt32 /*databaseId*/, std::unique_ptr<ServerEnvironment>> m_databaseEnvironments;
			Nz::Bitset<> m_disconnectedPlayers;
			Nz::Bitset<> m_newPlayers;
			Nz::MemoryPool<ServerPlayer> m_players;
			Nz::MillisecondClock m_saveClock;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
			Nz::TimerManager m_tickedTimerManager;
			Nz::UInt16 m_tickIndex;
			Nz::ApplicationBase& m_application;
			BlockLibrary m_blockLibrary;
			Config m_config;
			ScriptingContext m_scriptingContext;
			EntityRegistry m_entityRegistry;
			ServerDatabase m_serverDatabase;
			Spawnpoint m_defaultSpawnpoint;
	};
}

#include <ServerLib/ServerInstance.inl>

#endif // TSOM_SERVERLIB_SERVERINSTANCE_HPP
