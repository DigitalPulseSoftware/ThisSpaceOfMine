// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline DebugDrawInterface* ServerEnvironment::GetDebugDrawInterface()
	{
		return m_debugDrawer.get();
	}

	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback)
	{
		m_registeredPlayers.ForEachPlayer(std::forward<F>(callback));
	}

	template<typename F>
	void ServerEnvironment::ForEachPlayer(F&& callback) const
	{
		m_registeredPlayers.ForEachPlayer(std::forward<F>(callback));
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
