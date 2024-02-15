// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename... Args>
	NetworkSessionManager& ServerInstance::AddSessionManager(Args&& ...args)
	{
		return *m_sessionManagers.emplace_back(std::make_unique<NetworkSessionManager>(std::forward<Args>(args)...));
	}

	template<typename F> void ServerInstance::ForEachPlayer(F&& functor)
	{
		for (ServerPlayer& serverPlayer : m_players)
			functor(serverPlayer);
	}

	template<typename F> void ServerInstance::ForEachPlayer(F&& functor) const
	{
		for (const ServerPlayer& serverPlayer : m_players)
			functor(serverPlayer);
	}

	inline Planet& ServerInstance::GetPlanet()
	{
		return *m_planet;
	}

	inline const Planet& ServerInstance::GetPlanet() const
	{
		return *m_planet;
	}

	inline Nz::EnttWorld& ServerInstance::GetWorld()
	{
		return m_world;
	}
}
