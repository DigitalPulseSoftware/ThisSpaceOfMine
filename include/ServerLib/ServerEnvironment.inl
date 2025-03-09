// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Core/Error.hpp>

namespace tsom
{
	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback)
	{
		for (std::size_t playerIndex : m_registeredPlayers.IterBits())
			callback(*m_serverInstance.GetPlayer(playerIndex));
	}

	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback) const
	{
		for (std::size_t playerIndex : m_registeredPlayers.IterBits())
			callback(*m_serverInstance.GetPlayer(playerIndex));
	}

	inline ServerInstance& ServerEnvironment::GetServerInstance()
	{
		return m_serverInstance;
	}

	inline ServerEnvironmentType ServerEnvironment::GetType() const
	{
		return m_type;
	}

	inline Nz::EnttWorld& ServerEnvironment::GetWorld()
	{
		return *m_world;
	}

	inline const Nz::EnttWorld& ServerEnvironment::GetWorld() const
	{
		return *m_world;
	}

	inline bool ServerEnvironment::IsRoot() const
	{
		return m_isRoot;
	}

	inline ServerEnvironment* ServerEnvironment::GetEnvironment(entt::handle entity)
	{
		NazaraAssert(entity.registry());
		return GetEnvironment(*entity.registry());
	}

	inline ServerEnvironment* ServerEnvironment::GetEnvironment(entt::registry& registry)
	{
		return registry.ctx().get<ServerEnvironment*>();
	}

	inline void ServerEnvironment::ClearEntities()
	{
		m_world->GetRegistry().clear();
	}
}
