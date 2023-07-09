// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
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
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthRequest>,        { 0, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::UpdatePlayerInputs>, { 1, 0 } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session, Nz::EnttWorld& world) :
	SessionHandler(session),
	m_world(world)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
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

			if (entityData.playerControlled)
				SetupEntity(entity, std::move(entityData.playerControlled.value()));

			m_networkIdToEntity[entityData.entityId] = entity;

			if (entityData.playerControlled)
			{
				m_playerControlledEntity = entity;
				OnControlledEntityChanged(entity);
			}

			fmt::print("Create {}\n", entityData.entityId);
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

	void ClientSessionHandler::SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData)
	{
		auto collider = std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);

		std::shared_ptr<Nz::Model> colliderModel;
		{
			std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::Graphics::Instance()->GetDefaultMaterials().basicMaterial->Instantiate();
			colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
			colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
			{
				states.primitiveMode = Nz::PrimitiveMode::LineList;
				return true;
			});

			std::shared_ptr<Nz::Mesh> colliderMesh = Nz::Mesh::Build(collider->GenerateDebugMesh());
			std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*colliderMesh);

			colliderModel = std::make_shared<Nz::Model>(colliderGraphicalMesh);
			for (std::size_t i = 0; i < colliderModel->GetSubMeshCount(); ++i)
				colliderModel->SetMaterial(i, colliderMat);

			auto& gfx = entity.emplace<Nz::GraphicsComponent>();
			gfx.AttachRenderable(std::move(colliderModel), 0x0000FFFF);
		}

		entt::handle textEntity = m_world.CreateEntity();
		{
			auto& textNode = textEntity.emplace<Nz::NodeComponent>();
			textNode.SetParent(entity);
			textNode.SetInheritRotation(false);

			std::shared_ptr<Nz::TextSprite> textSprite = std::make_shared<Nz::TextSprite>();
			textSprite->Update(Nz::SimpleTextDrawer::Draw(entityData.nickname, 48), 0.01f);

			textEntity.emplace<Nz::GraphicsComponent>(std::move(textSprite));
		}

		entity.get_or_emplace<EntityOwnerComponent>().Register(textEntity);
	}
}
