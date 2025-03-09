// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerEnvironment.hpp>
#include <CommonLib/Physics/PhysicsSettings.hpp>
#include <CommonLib/Systems/TickSystem.hpp>
#include <ServerLib/Components/AtmosphereCarrier.hpp>
#include <ServerLib/Debug/DebugDrawBroadcaster.hpp>
#include <ServerLib/Systems/AtmosphereSystem.hpp>
#include <ServerLib/Systems/EnvironmentProxySystem.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	ServerEnvironment::ServerEnvironment(ServerInstance& serverInstance, ServerEnvironmentType type, bool isRoot) :
	m_type(type),
	m_serverInstance(serverInstance),
	m_isRoot(isRoot)
	{
		m_world = m_serverInstance.RegisterEnvironment(this);

		if (serverInstance.GetConfig().enableDebugDrawer)
			m_debugDrawer = std::make_unique<DebugDrawBroadcaster>(this);

		auto& registry = m_world->GetRegistry();
		registry.ctx().insert_or_assign<ServerEnvironment*>(this);

		m_world->AddSystem<AtmosphereSystem>();
		m_world->AddSystem<EnvironmentProxySystem>();
		m_world->AddSystem<NetworkedEntitiesSystem>(*this);
		m_world->AddSystem<TickSystem>();

		// Setup physics
		Nz::Physics3DSystem::Settings physSettings = Physics::BuildSettings();
		physSettings.stepSize = m_serverInstance.GetTickDuration();

		auto& physicsSystem = m_world->AddSystem<Nz::Physics3DSystem>(std::move(physSettings));
		physicsSystem.SetContactListener(Physics::BuildContactListener());
	}

	ServerEnvironment::~ServerEnvironment()
	{
		// Destroy all entities first
		auto& registry = m_world->GetRegistry();
		registry.clear();

		ForEachPlayer([this](ServerPlayer& player)
		{
			player.RemoveFromEnvironment(this);
		});

		m_world->ClearSystems();
		m_serverInstance.UnregisterEnvironment(this, std::move(m_world));
	}

	entt::handle ServerEnvironment::CreateEntity()
	{
		return m_world->CreateEntity();
	}

	void ServerEnvironment::ForEachAtmosphere(Nz::FunctionRef<void(ServerAtmosphere*)> callback)
	{
		auto& registry = m_world->GetRegistry();
		auto carrierView = registry.view<Nz::NodeComponent, AtmosphereCarrier>(entt::exclude<Nz::DisabledComponent>);

		for (entt::entity carrierEntity : carrierView)
		{
			AtmosphereCarrier& carrier = carrierView.get<AtmosphereCarrier>(carrierEntity);
			callback(carrier.atmosphere);
		}
	}

	void ServerEnvironment::ForEachAtmosphere(Nz::FunctionRef<void(const ServerAtmosphere*)> callback) const
	{
		auto& registry = m_world->GetRegistry();
		auto carrierView = registry.view<Nz::NodeComponent, AtmosphereCarrier>(entt::exclude<Nz::DisabledComponent>);

		for (entt::entity carrierEntity : carrierView)
		{
			AtmosphereCarrier& carrier = carrierView.get<AtmosphereCarrier>(carrierEntity);
			callback(carrier.atmosphere);
		}
	}

	ServerAtmosphere* ServerEnvironment::GetAtmosphereAtPosition(const Nz::Vector3f& position)
	{
		auto& registry = m_world->GetRegistry();
		auto carrierView = registry.view<Nz::NodeComponent, AtmosphereCarrier>(entt::exclude<Nz::DisabledComponent>);

		for (auto&& [carrierEntity, carrierNode, carrier] : carrierView.each())
		{
			Nz::Vector3f localPos = carrierNode.ToLocalPosition(position);

			// Use AABB as a cheap test
			if NAZARA_LIKELY(!carrier.aabb.Contains(localPos))
				continue;

			if (carrier.collider)
			{
				if (!carrier.collider->CollisionQuery(localPos - carrier.collider->GetCenterOfMass())) //< https://jrouwe.github.io/JoltPhysics/index.html#center-of-mass
					continue;
			}

			return carrier.atmosphere;
		}

		return GetFallbackAtmosphereAtPosition(position);
	}

	void ServerEnvironment::OnTick(Nz::Time elapsedTime)
	{
		m_world->Update(elapsedTime);
	}

	void ServerEnvironment::RegisterPlayer(ServerPlayer* player)
	{
		NazaraAssertMsg(!m_registeredPlayers.UnboundedTest(player->GetPlayerIndex()), "player was already registered");
		m_registeredPlayers.UnboundedSet(player->GetPlayerIndex());
	}

	void ServerEnvironment::UnregisterPlayer(ServerPlayer* player)
	{
		NazaraAssertMsg(m_registeredPlayers.UnboundedTest(player->GetPlayerIndex()), "player is not registered");
		m_registeredPlayers.Reset(player->GetPlayerIndex());
	}
}
