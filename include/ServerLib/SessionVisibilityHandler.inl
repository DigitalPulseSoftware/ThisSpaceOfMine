// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	inline SessionVisibilityHandler::SessionVisibilityHandler(NetworkSession* networkSession) :
	m_lastInputIndex(0),
	m_controlledCharacter(nullptr),
	m_networkSession(networkSession)
	{
		m_activeChunkUpdates = std::make_shared<std::size_t>(0);
	}

	inline void SessionVisibilityHandler::UpdateControlledEntity(entt::handle entity, CharacterController* controller)
	{
		if (m_controlledEntity)
			m_movingEntities.insert(m_controlledEntity);

		m_controlledEntity = entity;
		m_controlledCharacter = controller;
		m_movingEntities.erase(m_controlledEntity);
	}

	inline void SessionVisibilityHandler::UpdateLastInputIndex(InputIndex inputIndex)
	{
		m_lastInputIndex = inputIndex;
	}

	inline std::size_t SessionVisibilityHandler::HandlerHasher::operator()(const entt::handle& handle) const
	{
		std::size_t seed = std::hash<entt::registry*>{}(handle.registry());
		Nz::HashCombine(seed, handle.entity());
		return seed;
	}
}
