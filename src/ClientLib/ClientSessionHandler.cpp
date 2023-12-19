// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/Components/MovementInterpolationComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/TextSprite.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <Nazara/JoltPhysics3D/Components/JoltRigidBody3DComponent.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthRequest>,        { 0, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::MineBlock>,          { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::PlaceBlock>,         { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::SendChatMessage>,    { 0, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::UpdatePlayerInputs>, { 1, 0 } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session, Nz::EnttWorld& world) :
	SessionHandler(session),
	m_world(world),
	m_ownPlayerIndex(InvalidPlayerIndex),
	m_lastInputIndex(0)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
	}

	void ClientSessionHandler::HandlePacket(Packets::AuthResponse&& authResponse)
	{
		fmt::print("Auth response\n");
		m_ownPlayerIndex = authResponse.ownPlayerIndex;
	}

	void ClientSessionHandler::HandlePacket(Packets::ChatMessage&& chatMessage)
	{
		if (chatMessage.playerIndex)
		{
			if (chatMessage.playerIndex >= m_players.size())
			{
				fmt::print(fg(fmt::color::red), "ChatMessage with unknown player index {}\n", *chatMessage.playerIndex);
				return;
			}

			OnChatMessage(chatMessage.message, m_players[*chatMessage.playerIndex]->nickname);
		}
		else
			OnChatMessage(chatMessage.message, {});
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkCreate&& chunkCreate)
	{
		OnChunkCreate(chunkCreate);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkDestroy&& chunkDestroy)
	{
		OnChunkDestroy(chunkDestroy);
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesCreation&& entitiesCreation)
	{
		for (auto&& entityData : entitiesCreation.entities)
		{
			entt::handle entity = m_world.CreateEntity();
			entity.emplace<Nz::NodeComponent>(entityData.initialStates.position, entityData.initialStates.rotation);

			m_networkIdToEntity[entityData.entityId] = entity;

			if (entityData.playerControlled)
				SetupEntity(entity, std::move(entityData.playerControlled.value()));

			fmt::print("Create entity {}\n", entityData.entityId);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesDelete&& entitiesDelete)
	{
		for (auto entityId : entitiesDelete.entities)
		{
			entt::handle entity = m_networkIdToEntity[entityId];

			if (m_playerControlledEntity == entity)
				OnControlledEntityChanged({});

			entity.destroy();
			fmt::print("Delete entity {}\n", entityId);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesStateUpdate&& stateUpdate)
	{
		if (stateUpdate.lastInputIndex != m_lastInputIndex)
		{
			OnInputHandled(stateUpdate.lastInputIndex);
			m_lastInputIndex = stateUpdate.lastInputIndex;
		}

		for (auto& entityData : stateUpdate.entities)
		{
			entt::handle& entity = m_networkIdToEntity[entityData.entityId];

			if (MovementInterpolationComponent* movementInterpolation = entity.try_get<MovementInterpolationComponent>())
				movementInterpolation->PushMovement(stateUpdate.tickIndex, entityData.newStates.position, entityData.newStates.rotation);
			else
			{
				auto& entityNode = entity.get<Nz::NodeComponent>();
				entityNode.SetTransform(entityData.newStates.position, entityData.newStates.rotation);
			}
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::GameData&& gameData)
	{
		m_lastTickIndex = gameData.tickIndex;
		for (auto& playerData : gameData.players)
		{
			if (playerData.index >= m_players.size())
				m_players.resize(playerData.index + 1);

			auto& playerInfo = m_players[playerData.index].emplace();
			playerInfo.nickname = std::move(playerData.nickname);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::PlayerLeave&& playerLeave)
	{
		if (playerLeave.index >= m_players.size())
		{
			fmt::print(fg(fmt::color::red), "PlayerLeave with unknown player index {}\n", playerLeave.index);
			return;
		}

		OnPlayerLeave(m_players[playerLeave.index]->nickname);

		m_players[playerLeave.index].reset();
	}

	void ClientSessionHandler::HandlePacket(Packets::PlayerJoin&& playerJoin)
	{
		if (playerJoin.index >= m_players.size())
			m_players.resize(playerJoin.index + 1);

		auto& playerInfo = m_players[playerJoin.index].emplace();
		playerInfo.nickname = std::move(playerJoin.nickname);

		OnPlayerJoined(playerInfo.nickname);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkUpdate&& stateUpdate)
	{
		OnChunkUpdate(stateUpdate);
	}

	void ClientSessionHandler::SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData)
	{
		//entity.emplace<MovementInterpolationComponent>(m_lastTickIndex);

		auto collider = std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);
		entity.emplace<Nz::JoltRigidBody3DComponent>(Nz::JoltRigidBody3D::DynamicSettings(collider, 0.f));

		if (entityData.controllingPlayerId == m_ownPlayerIndex)
		{
			m_playerControlledEntity = entity;
			OnControlledEntityChanged(entity);
		}
		else
		{
			// Player model (collider for now)
			std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
			colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.primitiveMode = Nz::PrimitiveMode::LineList;
				return true;
			});

			std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(collider->GenerateDebugMesh());
			std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

			std::shared_ptr<Nz::Model> colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
			for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
				colliderModel->SetMaterial(i, colliderMat);

			auto& gfx = entity.emplace<Nz::GraphicsComponent>();
			gfx.AttachRenderable(std::move(colliderModel), 0x0000FFFF);

			// Floating name
			std::shared_ptr<Nz::TextSprite> textSprite = std::make_shared<Nz::TextSprite>();

			if (const PlayerInfo* playerInfo = FetchPlayerInfo(entityData.controllingPlayerId))
				textSprite->Update(Nz::SimpleTextDrawer::Draw(playerInfo->nickname, 48), 0.01f);
			else
				textSprite->Update(Nz::SimpleTextDrawer::Draw("<disconnected>", 48), 0.01f);

			entt::handle frontTextEntity = m_world.CreateEntity();
			{
				auto& textNode = frontTextEntity.emplace<Nz::NodeComponent>();
				textNode.SetParent(entity);
				textNode.SetPosition(-textSprite->GetAABB().width * 0.5f, 1.5f, 0.f);

				frontTextEntity.emplace<Nz::GraphicsComponent>(textSprite);
			}
			entity.get_or_emplace<EntityOwnerComponent>().Register(frontTextEntity);

			entt::handle backTextEntity = m_world.CreateEntity();
			{
				auto& textNode = backTextEntity.emplace<Nz::NodeComponent>();
				textNode.SetParent(entity);
				textNode.SetPosition(textSprite->GetAABB().width * 0.5f, 1.5f, 0.f);
				textNode.SetRotation(Nz::EulerAnglesf(0.f, Nz::TurnAnglef(0.5f), 0.f));

				backTextEntity.emplace<Nz::GraphicsComponent>(std::move(textSprite));
			}

			entity.get_or_emplace<EntityOwnerComponent>().Register(backTextEntity);
		}
	}
}
