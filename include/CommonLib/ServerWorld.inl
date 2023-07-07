#include "ServerWorld.hpp"
// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename... Args>
	NetworkSessionManager& ServerWorld::AddSessionManager(Args&& ...args)
	{
		return *m_sessionManagers.emplace_back(std::make_unique<NetworkSessionManager>(std::forward<Args>(args)...));
	}

	template<typename F> void ServerWorld::ForEachPlayer(F&& functor)
	{
		for (ServerPlayer& serverPlayer : m_players)
			functor(serverPlayer);
	}

	template<typename F> void ServerWorld::ForEachPlayer(F&& functor) const
	{
		for (const ServerPlayer& serverPlayer : m_players)
			functor(serverPlayer);
	}

	inline Planet& ServerWorld::GetPlanet()
	{
		return *m_planet;
	}

	inline const Planet& ServerWorld::GetPlanet() const
	{
		return *m_planet;
	}

	inline Nz::EnttWorld& ServerWorld::GetWorld()
	{
		return m_world;
	}
}
