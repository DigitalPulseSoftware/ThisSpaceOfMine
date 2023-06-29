// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ServerWorld.hpp>
#include <memory>

namespace tsom
{
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
		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();

		m_world.Update(elapsedTime);
	}
}
