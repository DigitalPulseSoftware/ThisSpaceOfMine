// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERINSTANCE_HPP
#define TSOM_SERVERLIB_SERVERINSTANCE_HPP

#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Systems/PlanetGravitySystem.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/MemoryPool.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <memory>
#include <unordered_set>
#include <vector>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerInstance
	{
		public:
			ServerInstance();
			ServerInstance(const ServerInstance&) = delete;
			ServerInstance(ServerInstance&&) = delete;
			~ServerInstance();

			template<typename... Args> NetworkSessionManager& AddSessionManager(Args&&... args);

			void BroadcastChatMessage(std::string message, std::optional<PlayerIndex> senderIndex);

			ServerPlayer* CreatePlayer(NetworkSession* session, std::string nickname);
			void DestroyPlayer(PlayerIndex playerIndex);

			template<typename F> void ForEachPlayer(F&& functor);
			template<typename F> void ForEachPlayer(F&& functor) const;

			inline Planet& GetPlanet();
			inline const Planet& GetPlanet() const;
			inline Nz::EnttWorld& GetWorld();

			Nz::Time Update(Nz::Time elapsedTime);

			ServerInstance& operator=(const ServerInstance&) = delete;
			ServerInstance& operator=(ServerInstance&&) = delete;

		private:
			void LoadChunks();
			void OnNetworkTick();
			void OnTick(Nz::Time elapsedTime);
			void OnSave();

			struct BlockUpdate
			{
				Nz::Vector3ui chunkIndices;
				Nz::Vector3ui voxelIndices;
				BlockIndex newBlock;
			};

			Nz::UInt16 m_tickIndex;
			std::unique_ptr<Planet> m_planet;
			std::unique_ptr<ChunkEntities> m_planetEntities;
			std::unordered_set<Nz::Vector3ui /*chunkIndex*/> m_dirtyChunks;
			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
			Nz::Bitset<> m_disconnectedPlayers;
			Nz::Bitset<> m_newPlayers;
			Nz::EnttWorld m_world;
			Nz::MemoryPool<ServerPlayer> m_players;
			Nz::MillisecondClock m_saveClock;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
			BlockLibrary m_blockLibrary;
			PlanetGravitySystem m_gravitySystem;
	};
}

#include <ServerLib/ServerInstance.inl>

#endif
