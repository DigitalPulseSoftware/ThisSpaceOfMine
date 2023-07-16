// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ServerInstance.hpp>
#include <CommonLib/NetworkedEntitiesSystem.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <Nazara/Utility/Components.hpp>
#include <fmt/format.h>
#include <fmt/std.h>
#include <memory>

namespace tsom
{
	ServerInstance::ServerInstance() :
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Nz::Time::TickDuration(30))
	{
		m_world.AddSystem<NetworkedEntitiesSystem>(*this);
		auto& physicsSystem = m_world.AddSystem<Nz::JoltPhysics3DSystem>();
		physicsSystem.GetPhysWorld().SetStepSize(m_tickDuration);
		physicsSystem.GetPhysWorld().SetGravity(Nz::Vector3f::Zero());

		m_planet = std::make_unique<Planet>(40, 2.f, 16.f);

		
		m_planetEntity = m_world.CreateEntity();
		{
			m_planetEntity.emplace<Nz::NodeComponent>();

			Nz::JoltRigidBody3D::StaticSettings settings;
			{
				Nz::HighPrecisionClock colliderClock;
				settings.geom = m_planet->BuildCollider();
				fmt::print("built collider in {}\n", fmt::streamed(colliderClock.GetElapsedTime()));
			}

			auto& planetBody = m_planetEntity.emplace<Nz::JoltRigidBody3DComponent>(physicsSystem.CreateRigidBody(settings));
		}
	}

	ServerPlayer* ServerInstance::CreatePlayer(NetworkSession* session, std::string nickname)
	{
		std::size_t playerIndex;

		// defer construct so player can be constructed with their index
		ServerPlayer* player = m_players.Allocate(m_players.DeferConstruct, playerIndex);
		std::construct_at(player, *this, playerIndex, session, std::move(nickname));

		return player;
	}

	void ServerInstance::DestroyPlayer(std::size_t playerIndex)
	{
		m_players.Free(playerIndex);
	}

	void ServerInstance::Update(Nz::Time elapsedTime)
	{
		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			OnTick(m_tickDuration);
			m_tickAccumulator -= m_tickDuration;
		}
	}

	void ServerInstance::OnNetworkTick()
	{
		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.GetVisibilityHandler().Dispatch();
		});
	}

	void ServerInstance::OnTick(Nz::Time elapsedTime)
	{
		OnNetworkTick();

		m_world.Update(elapsedTime);
	}
}
