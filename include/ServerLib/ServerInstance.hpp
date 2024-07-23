// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERINSTANCE_HPP
#define TSOM_SERVERLIB_SERVERINSTANCE_HPP

#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/Planet.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EnttWorld.hpp>
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
}

namespace tsom
{
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

			ServerPlayer* CreateAnonymousPlayer(NetworkSession* session, std::string nickname);
			ServerPlayer* CreateAuthenticatedPlayer(NetworkSession* session, const Nz::Uuid& uuid, std::string nickname, PlayerPermissionFlags permissions);
			void DestroyPlayer(PlayerIndex playerIndex);

			inline ServerPlayer* FindPlayerByNickname(std::string_view nickname);
			inline const ServerPlayer* FindPlayerByNickname(std::string_view nickname) const;
			inline ServerPlayer* FindPlayerByUuid(const Nz::Uuid& uuid);
			inline const ServerPlayer* FindPlayerByUuid(const Nz::Uuid& uuid) const;

			template<typename F> void ForEachPlayer(F&& functor);
			template<typename F> void ForEachPlayer(F&& functor) const;

			inline Nz::ApplicationBase& GetApp();
			inline const BlockLibrary& GetBlockLibrary() const;
			inline const std::array<std::uint8_t, 32>& GetConnectionTokenEncryptionKey() const;
			inline Planet& GetPlanet();
			inline const Planet& GetPlanet() const;
			inline Nz::EnttWorld& GetWorld();

			Nz::Time Update(Nz::Time elapsedTime);

			ServerInstance& operator=(const ServerInstance&) = delete;
			ServerInstance& operator=(ServerInstance&&) = delete;

			struct Config
			{
				std::filesystem::path saveDirectory = Nz::Utf8Path("save/chunks");
				std::array<std::uint8_t, 32> connectionTokenEncryptionKey;
				Nz::Time saveInterval = Nz::Time::Seconds(30);
				Nz::UInt32 planetSeed = 42;
				Nz::Vector3ui planetChunkCount = Nz::Vector3ui(5);
				bool pauseWhenEmpty = true;
			};

		private:
			void LoadChunks();
			void OnNetworkTick();
			void OnTick(Nz::Time elapsedTime);
			void OnSave();

			struct PlayerRename
			{
				PlayerIndex playerIndex;
				std::string newNickname;
			};

			std::array<std::uint8_t, 32> m_connectionTokenEncryptionKey;
			std::filesystem::path m_saveDirectory;
			std::unique_ptr<Planet> m_planet;
			std::unique_ptr<ChunkEntities> m_planetEntities;
			std::unordered_set<ChunkIndices /*chunkIndex*/> m_dirtyChunks;
			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
			std::vector<PlayerRename> m_pendingPlayerRename;
			Nz::Bitset<> m_disconnectedPlayers;
			Nz::Bitset<> m_newPlayers;
			Nz::EnttWorld m_world;
			Nz::MemoryPool<ServerPlayer> m_players;
			Nz::MillisecondClock m_saveClock;
			Nz::Time m_saveInterval;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
			Nz::UInt16 m_tickIndex;
			BlockLibrary m_blockLibrary;
			Nz::ApplicationBase& m_application;
			bool m_pauseWhenEmpty;
	};
}

#include <ServerLib/ServerInstance.inl>

#endif // TSOM_SERVERLIB_SERVERINSTANCE_HPP
