// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Graphics/TextSprite.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable m_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthRequest>, { 0, Nz::ENetPacketFlag_Reliable } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session, Nz::EnttWorld& world) :
	SessionHandler(session),
	m_world(world)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(m_packetAttributes);
	}

	void ClientSessionHandler::HandlePacket(Packets::AuthResponse&& authResponse)
	{
		fmt::print("Auth response\n");
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesCreation&& entitiesCreation)
	{
		for (auto&& entityData : entitiesCreation.entities)
		{
			entt::handle entity = m_world.CreateEntity();
			entity.emplace<Nz::NodeComponent>(entityData.initialStates.position, entityData.initialStates.rotation);

			std::shared_ptr<Nz::TextSprite> textSprite = std::make_shared<Nz::TextSprite>();
			textSprite->Update(Nz::SimpleTextDrawer::Draw("Entity #" + std::to_string(entityData.entityId), 48), 0.01f);

			entity.emplace<Nz::GraphicsComponent>(std::move(textSprite));

			m_networkIdToEntity[entityData.entityId] = entity;

			fmt::print("Create {}\n", entityData.entityId);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesDelete&& entitiesDelete)
	{
		for (auto entityId : entitiesDelete.entities)
		{
			m_networkIdToEntity[entityId].destroy();

			fmt::print("Delete {}\n", entityId);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesStateUpdate&& stateUpdate)
	{
		for (auto& entityData : stateUpdate.entities)
		{
			fmt::print("New position of entity #{} = {};{};{}\n", entityData.entityId, entityData.newStates.position.x, entityData.newStates.position.y, entityData.newStates.position.z);

			auto& entityNode = m_networkIdToEntity[entityData.entityId].get<Nz::NodeComponent>();
			entityNode.SetTransform(entityData.newStates.position, entityData.newStates.rotation);
		}
	}
}
