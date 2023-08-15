// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERINSTANCE_HPP
#define TSOM_SERVERLIB_SERVERINSTANCE_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/PlanetEntities.hpp>
#include <CommonLib/VoxelBlock.hpp>
#include <ServerLib/ServerPlayer.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <NazaraUtils/MemoryPool.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <memory>
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

			void Update(Nz::Time elapsedTime);
			void UpdatePlanetBlock(const Nz::Vector3ui& chunkIndices, const Nz::Vector3ui& voxelIndices, VoxelBlock newBlock);

			ServerInstance& operator=(const ServerInstance&) = delete;
			ServerInstance& operator=(ServerInstance&&) = delete;

		private:
			void OnNetworkTick();
			void OnTick(Nz::Time elapsedTime);

			struct BlockUpdate
			{
				Nz::Vector3ui chunkIndices;
				Nz::Vector3ui voxelIndices;
				VoxelBlock newBlock;
			};

			Nz::UInt16 m_tickIndex;
			std::unique_ptr<Planet> m_planet;
			std::unique_ptr<PlanetEntities> m_planetEntities;
			std::vector<BlockUpdate> m_voxelGridUpdates;
			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
			Nz::Bitset<> m_disconnectedPlayers;
			Nz::Bitset<> m_newPlayers;
			Nz::EnttWorld m_world;
			Nz::MemoryPool<ServerPlayer> m_players;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
	};
}

#include <ServerLib/ServerInstance.inl>

#endif
