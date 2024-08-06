// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerEnvironment.hpp>
#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <ServerLib/SessionVisibilityHandler.hpp>
#include <ServerLib/Systems/EnvironmentProxySystem.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <ServerLib/Systems/TempShipEntrySystem.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <cassert>

namespace tsom
{
	ServerEnvironment::ServerEnvironment(ServerInstance& serverInstance) :
	m_serverInstance(serverInstance)
	{
		auto& registry = m_world.GetRegistry();
		registry.ctx().emplace<ServerEnvironment*>(this);

		m_world.AddSystem<EnvironmentProxySystem>();
		m_world.AddSystem<NetworkedEntitiesSystem>(*this);

		// Setup physics
		Nz::Physics3DSystem::Settings physSettings = Physics::BuildSettings();
		physSettings.stepSize = m_serverInstance.GetTickDuration();

		m_world.AddSystem<Nz::Physics3DSystem>(std::move(physSettings));
	}

	ServerEnvironment::~ServerEnvironment() = default;

	void ServerEnvironment::Connect(ServerEnvironment& environment, const EnvironmentTransform& transform)
	{
		NazaraAssert(!m_connectedEnvironments.contains(&environment), "environment is already connected");
		m_connectedEnvironments.emplace(&environment, transform);
	}

	void ServerEnvironment::Disconnect(ServerEnvironment& environment)
	{
		auto it = m_connectedEnvironments.find(&environment);
		NazaraAssert(it != m_connectedEnvironments.end(), "environment is not connected");
		m_connectedEnvironments.erase(it);
	}

	entt::handle ServerEnvironment::CreateEntity()
	{
		return m_world.CreateEntity();
	}

	void ServerEnvironment::OnTick(Nz::Time elapsedTime)
	{
		m_world.Update(elapsedTime);
	}

	void ServerEnvironment::RegisterPlayer(ServerPlayer* player)
	{
		NazaraAssert(!m_registeredPlayers.UnboundedTest(player->GetPlayerIndex()), "player was already registered");
		m_registeredPlayers.UnboundedSet(player->GetPlayerIndex());
	}

	void ServerEnvironment::UnregisterPlayer(ServerPlayer* player)
	{
		NazaraAssert(m_registeredPlayers.UnboundedTest(player->GetPlayerIndex()), "player is not registered");
		m_registeredPlayers.Reset(player->GetPlayerIndex());
	}

	void ServerEnvironment::UpdateConnectedTransform(ServerEnvironment& environment, const EnvironmentTransform& transform)
	{
		auto it = m_connectedEnvironments.find(&environment);
		NazaraAssert(it != m_connectedEnvironments.end(), "unknown environment");
		it.value() = transform;
	}
}
