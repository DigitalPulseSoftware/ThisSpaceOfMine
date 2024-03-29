// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	ServerEnvironment::ServerEnvironment(ServerInstance& serverInstance) :
	m_serverInstance(serverInstance)
	{
		m_world.AddSystem<NetworkedEntitiesSystem>(*this);
		auto& physicsSystem = m_world.AddSystem<Nz::Physics3DSystem>();
		{
			auto& physWorld = physicsSystem.GetPhysWorld();
			physWorld.SetStepSize(m_serverInstance.GetTickDuration());
			physWorld.SetGravity(Nz::Vector3f::Zero());
		}
	}

	ServerEnvironment::~ServerEnvironment() = default;

	entt::handle ServerEnvironment::CreateEntity()
	{
		return m_world.CreateEntity();
	}

	void ServerEnvironment::OnTick(Nz::Time elapsedTime)
	{
		m_world.Update(elapsedTime);
	}
}
