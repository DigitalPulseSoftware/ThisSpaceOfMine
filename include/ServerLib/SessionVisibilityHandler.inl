// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	inline SessionVisibilityHandler::SessionVisibilityHandler(NetworkSession* networkSession) :
	m_currentEnvironmentId(Nz::MaxValue()),
	m_lastInputIndex(0),
	m_controlledCharacter(nullptr),
	m_networkSession(networkSession)
	{
		m_activeChunkUpdates = std::make_shared<std::size_t>(0);
	}

	inline bool SessionVisibilityHandler::GetChunkByNetworkId(Packets::Helper::ChunkId networkId, entt::handle* entityOwner, Chunk** chunk) const
	{
		if (networkId >= m_visibleChunks.size())
			return false;

		auto& chunkData = m_visibleChunks[networkId];
		if (!chunkData.chunk)
			return false;

		if (entityOwner)
			*entityOwner = chunkData.entityOwner;

		if (chunk)
			*chunk = chunkData.chunk;

		return true;
	}

	inline bool SessionVisibilityHandler::GetEntityByNetworkId(Packets::Helper::EntityId networkId, entt::handle* entity) const
	{
		if (networkId >= m_visibleEntities.size())
			return false;

		auto& entityData = m_visibleEntities[networkId];
		if (!entityData.entity)
			return false;

		*entity = entityData.entity;
		return true;
	}

	inline Packets::Helper::EnvironmentId SessionVisibilityHandler::GetEnvironmentId(ServerEnvironment* environment) const
	{
		return Nz::Retrieve(m_environmentIndices, environment);
	}

	inline void SessionVisibilityHandler::TriggerEntityRpc(entt::handle entity, Nz::UInt32 rpcIndex)
	{
		m_triggeredEntitiesRpc[entity].push_back(rpcIndex);
	}

	inline void SessionVisibilityHandler::UpdateControlledEntity(entt::handle entity, CharacterController* controller)
	{
		if (m_controlledEntity)
			m_movingEntities.insert(m_controlledEntity);

		m_controlledEntity = entity;
		m_controlledCharacter = controller;
		m_movingEntities.erase(m_controlledEntity);
	}

	inline void SessionVisibilityHandler::UpdateEntityProperty(entt::handle entity, Nz::UInt32 propertyIndex, const EntityProperty& newValue)
	{
		Nz::UInt32 propertyMask = 1u << propertyIndex;

		auto& propertyUpdateData = m_propertyUpdatedEntities[entity];
		std::size_t propertyValueIndex = Nz::CountBits(propertyUpdateData.propertiesMask & (propertyMask - 1));

		if ((propertyUpdateData.propertiesMask & propertyMask) != 0)
		{
			// Update value (find value index and then update it)
			propertyUpdateData.values[propertyValueIndex] = newValue;
		}
		else
		{
			propertyUpdateData.propertiesMask |= propertyMask;
			propertyUpdateData.values.insert(propertyUpdateData.values.begin() + propertyValueIndex, newValue);
		}
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
