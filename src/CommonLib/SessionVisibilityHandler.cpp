// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/SessionVisibilityHandler.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <NazaraUtils/Algorithm.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>

namespace tsom
{
	void SessionVisibilityHandler::CreateEntity(entt::handle entity, bool isMoving)
	{
		m_createdEntities.emplace(entity);
		if (isMoving)
			m_movingEntities.emplace(entity);
	}

	void SessionVisibilityHandler::DestroyEntity(entt::handle entity)
	{
		m_createdEntities.erase(entity);
		m_movingEntities.erase(entity);
		m_deletedEntities.emplace(entity);
	}

	void SessionVisibilityHandler::Dispatch()
	{
		if (!m_deletedEntities.empty())
		{
			Packets::EntitiesDelete deletePacket;

			for (const entt::handle& handle : m_deletedEntities)
			{
				Nz::UInt32 entityId = Nz::Retrieve(m_entityToNetworkId, handle);
				deletePacket.entities.push_back(entityId);

				m_freeEntityIds.Set(entityId, true);

				m_entityToNetworkId.erase(handle);
			}

			m_networkSession->SendPacket(deletePacket);
			m_deletedEntities.clear();
		}

		if (!m_createdEntities.empty())
		{
			Packets::EntitiesCreation creationPacket;

			for (const entt::handle& handle : m_createdEntities)
			{
				std::size_t networkId = m_freeEntityIds.FindFirst();
				if (networkId == m_freeEntityIds.npos)
				{
					networkId = m_freeEntityIds.GetSize();
					m_freeEntityIds.Resize(networkId + FreeIdGrowRate, true);
				}

				m_freeEntityIds.Set(networkId, false);

				m_entityToNetworkId[handle] = networkId;

				auto& entityNode = handle.get<Nz::NodeComponent>();

				auto& entityData = creationPacket.entities.emplace_back();
				entityData.entityId = Nz::SafeCast<Nz::UInt32>(networkId);
				entityData.initialStates.position = entityNode.GetPosition();
				entityData.initialStates.rotation = entityNode.GetRotation();
			}

			m_networkSession->SendPacket(creationPacket);
			m_createdEntities.clear();
		}

		Packets::EntitiesStateUpdate stateUpdate;

		for (const entt::handle& handle : m_movingEntities)
		{
			auto& entityData = stateUpdate.entities.emplace_back();

			auto& entityNode = handle.get<Nz::NodeComponent>();

			entityData.entityId = Nz::Retrieve(m_entityToNetworkId, handle);

			entityData.newStates.position = entityNode.GetPosition();
			entityData.newStates.rotation = entityNode.GetRotation();
		}

		if (!stateUpdate.entities.empty())
			m_networkSession->SendPacket(stateUpdate);
	}
}
