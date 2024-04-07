// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	inline SessionVisibilityHandler::SessionVisibilityHandler(NetworkSession* networkSession) :
	m_currentEnvironmentId(std::numeric_limits<EnvironmentId>::max()),
	m_lastInputIndex(0),
	m_controlledCharacter(nullptr),
	m_networkSession(networkSession)
	{
		m_activeChunkUpdates = std::make_shared<std::size_t>(0);
	}

	inline Chunk* SessionVisibilityHandler::GetChunkByIndex(std::size_t chunkIndex) const
	{
		if (chunkIndex >= m_visibleChunks.size())
			return nullptr;

		return m_visibleChunks[chunkIndex].chunk;
	}

	inline void SessionVisibilityHandler::MoveEnvironment(ServerEnvironment& environment, const EnvironmentTransform& transform)
	{
		auto it = std::find_if(m_environmentTransformations.begin(), m_environmentTransformations.end(), [&](const EnvironmentTransformation& transform) { return transform.environment == &environment; });
		if (it == m_environmentTransformations.end())
		{
			m_environmentTransformations.push_back({
				.environment = &environment,
				.transform = transform
			});
		}
		else
		{
			EnvironmentTransformation& transformation = *it;
			transformation.transform = transform;
		}
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

	inline void SessionVisibilityHandler::UpdateRootEnvironment(ServerEnvironment& environment)
	{
		m_nextRootEnvironment = &environment;
	}

	inline std::size_t SessionVisibilityHandler::HandlerHasher::operator()(const entt::handle& handle) const
	{
		std::size_t seed = std::hash<entt::registry*>{}(handle.registry());
		Nz::HashCombine(seed, handle.entity());
		return seed;
	}
}
