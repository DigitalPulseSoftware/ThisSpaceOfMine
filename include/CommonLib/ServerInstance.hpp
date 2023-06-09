// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SERVERINSTANCE_HPP
#define TSOM_COMMONLIB_SERVERINSTANCE_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/NetworkSessionManager.hpp>
#include <CommonLib/ServerPlayer.hpp>
#include <NazaraUtils/MemoryPool.hpp>
#include <Nazara/Core/Clock.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <memory>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API ServerInstance
	{
		public:
			ServerInstance();
			ServerInstance(const ServerInstance&) = delete;
			ServerInstance(ServerInstance&&) = delete;
			~ServerInstance() = default;

			template<typename... Args> NetworkSessionManager& AddSessionManager(Args&&... args);

			ServerPlayer* CreatePlayer(NetworkSession* session, std::string nickname);
			void DestroyPlayer(std::size_t playerIndex);

			template<typename F> void ForEachPlayer(F&& functor);
			template<typename F> void ForEachPlayer(F&& functor) const;

			inline Planet& GetPlanet();
			inline const Planet& GetPlanet() const;
			inline Nz::EnttWorld& GetWorld();

			void Update(Nz::Time elapsedTime);

			ServerInstance& operator=(const ServerInstance&) = delete;
			ServerInstance& operator=(ServerInstance&&) = delete;

		private:
			void OnNetworkTick();
			void OnTick(Nz::Time elapsedTime);

			entt::handle m_planetEntity;
			std::unique_ptr<Planet> m_planet;
			std::vector<std::unique_ptr<NetworkSessionManager>> m_sessionManagers;
			Nz::EnttWorld m_world;
			Nz::MemoryPool<ServerPlayer> m_players;
			Nz::Time m_tickAccumulator;
			Nz::Time m_tickDuration;
	};
}

#include <CommonLib/ServerInstance.inl>

#endif
