// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ServerWorld.hpp>
#include <CommonLib/NetworkedEntitiesSystem.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <memory>

namespace tsom
{
	ServerWorld::ServerWorld() :
	m_players(256),
	m_tickAccumulator(Nz::Time::Zero()),
	m_tickDuration(Nz::Time::TickDuration(30))
	{
		m_planet = std::make_unique<Planet>(40, 2.f, 16.f);

		m_world.AddSystem<NetworkedEntitiesSystem>(*this);
		m_world.AddSystem<Nz::JoltPhysics3DSystem>();
	}

	ServerPlayer* ServerWorld::CreatePlayer(NetworkSession* session, std::string nickname)
	{
		std::size_t playerIndex;

		// defer construct so player can be constructed with their index
		ServerPlayer* player = m_players.Allocate(m_players.DeferConstruct, playerIndex);
		std::construct_at(player, *this, playerIndex, session, std::move(nickname));

		return player;
	}

	void ServerWorld::Update(Nz::Time elapsedTime)
	{
		m_tickAccumulator += elapsedTime;
		while (m_tickAccumulator >= m_tickDuration)
		{
			OnTick(m_tickDuration);
			m_tickAccumulator -= m_tickDuration;
		}
	}

	void ServerWorld::NetworkTick()
	{
		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			serverPlayer.GetVisibilityHandler().Dispatch();
		});
	}

	void ServerWorld::OnTick(Nz::Time elapsedTime)
	{
		NetworkTick();

		m_world.Update(elapsedTime);
	}
}
