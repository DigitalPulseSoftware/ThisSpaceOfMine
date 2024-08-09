// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/PlayerAnimationController.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/AnimationComponent.hpp>
#include <ClientLib/Components/MovementInterpolationComponent.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Core/Components/SkeletonComponent.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/PredefinedMaterials.hpp>
#include <Nazara/Graphics/TextSprite.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Graphics/PropertyHandler/TexturePropertyHandler.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthRequest>,        { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::MineBlock>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::PlaceBlock>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::SendChatMessage>,    { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::UpdatePlayerInputs>, { .channel = 1, .flags = Nz::ENetPacketFlag_Unreliable } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session, Nz::ApplicationBase& app, Nz::EnttWorld& world, ClientBlockLibrary& blockLibrary) :
	SessionHandler(session),
	m_app(app),
	m_world(world),
	m_blockLibrary(blockLibrary),
	m_ownPlayerIndex(InvalidPlayerIndex),
	m_lastInputIndex(0)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
	}

	ClientSessionHandler::~ClientSessionHandler()
	{
		for (auto it = m_networkIdToEntity.begin(); it != m_networkIdToEntity.end(); ++it)
		{
			if (entt::handle entity = it.value(); entity.valid())
				entity.destroy();
		}
		m_networkIdToEntity.clear();
	}

	void ClientSessionHandler::HandlePacket(Packets::AuthResponse&& authResponse)
	{
		if (authResponse.authResult.IsOk())
			m_ownPlayerIndex = authResponse.ownPlayerIndex;

		OnAuthResponse(authResponse);
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

			OnPlayerChatMessage(chatMessage.message, *m_players[*chatMessage.playerIndex]);
		}
		else
			OnChatMessage(chatMessage.message);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkCreate&& chunkCreate)
	{
		OnChunkCreate(chunkCreate);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkDestroy&& chunkDestroy)
	{
		OnChunkDestroy(chunkDestroy);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkReset&& chunkReset)
	{
		OnChunkReset(chunkReset);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkUpdate&& stateUpdate)
	{
		OnChunkUpdate(stateUpdate);
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

			if (entityData.ship)
				SetupEntity(entity, std::move(entityData.ship.value()));

			fmt::print("Created entity {}\n", entityData.entityId);
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

		if (stateUpdate.controlledCharacter)
			OnControlledEntityStateUpdate(stateUpdate.lastInputIndex, *stateUpdate.controlledCharacter);
	}

	void ClientSessionHandler::HandlePacket(Packets::GameData&& gameData)
	{
		m_lastTickIndex = gameData.tickIndex;
		for (auto& playerData : gameData.players)
		{
			if (playerData.index >= m_players.size())
				m_players.resize(playerData.index + 1);

			auto& playerInfo = m_players[playerData.index].emplace();
			playerInfo.nickname = std::move(playerData.nickname).Str();
			playerInfo.isAuthenticated = playerData.isAuthenticated;
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::PlayerJoin&& playerJoin)
	{
		if (playerJoin.index >= m_players.size())
			m_players.resize(playerJoin.index + 1);

		auto& playerInfo = m_players[playerJoin.index].emplace();
		playerInfo.nickname = std::move(playerJoin.nickname).Str();
		playerInfo.isAuthenticated = playerJoin.isAuthenticated;

		OnPlayerJoined(playerInfo);
	}

	void ClientSessionHandler::HandlePacket(Packets::PlayerLeave&& playerLeave)
	{
		if (playerLeave.index >= m_players.size() || !m_players[playerLeave.index])
		{
			fmt::print(fg(fmt::color::red), "PlayerLeave with unknown player index {}\n", playerLeave.index);
			return;
		}

		OnPlayerLeave(*m_players[playerLeave.index]);

		m_players[playerLeave.index].reset();
	}

	void ClientSessionHandler::HandlePacket(Packets::PlayerNameUpdate&& playerNameUpdate)
	{
		if (playerNameUpdate.index >= m_players.size() || !m_players[playerNameUpdate.index])
		{
			fmt::print(fg(fmt::color::red), "PlayerNameUpdate with unknown player index {}\n", playerNameUpdate.index);
			return;
		}

		auto& playerInfo = *m_players[playerNameUpdate.index];

		OnPlayerNameUpdate(playerInfo, playerNameUpdate.newNickname);
		playerInfo.nickname = std::move(playerNameUpdate.newNickname).Str();
		if (playerInfo.textSprite)
			playerInfo.textSprite->Update(Nz::SimpleTextDrawer::Draw(playerInfo.nickname, 48, Nz::TextStyle_Regular, (playerInfo.isAuthenticated) ? Nz::Color::White() : Nz::Color::Gray()), 0.01f);
	}

	void ClientSessionHandler::SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData)
	{
		auto collider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);
		entity.emplace<Nz::RigidBody3DComponent>(Nz::RigidBody3D::DynamicSettings(collider, 0.f));

		// Player model (collider for now)
		if (!m_playerModel)
		{
			m_playerModel.emplace();

			auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();

			m_playerAnimAssets = std::make_shared<PlayerAnimationAssets>();

			Nz::ModelParams params;
			params.loadMaterials = false;
			params.mesh.vertexDeclaration = Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Normal_UV_Tangent_Skinning);
			params.meshCallback = [&](const std::shared_ptr<Nz::Mesh>& mesh) -> Nz::Result<void, Nz::ResourceLoadingError>
			{
				if (!mesh->IsAnimable())
					return Nz::Err(Nz::ResourceLoadingError::Unrecognized);

				m_playerAnimAssets->referenceSkeleton = std::move(*mesh->GetSkeleton());
				return Nz::Ok();
			};

			params.mesh.vertexOffset = Nz::Vector3f(0.f, -0.826f, 0.f);
			params.mesh.vertexRotation = Nz::Quaternionf(Nz::TurnAnglef(0.5f), Nz::Vector3f::Up());
			params.mesh.vertexScale = Nz::Vector3f(1.f / 10.f);

			m_playerModel->model = fs.Load<Nz::Model>("assets/Player/Idle.fbx", params);
			if (m_playerModel->model)
			{
				assert(m_playerAnimAssets->referenceSkeleton.IsValid());

				Nz::AnimationParams animParams;
				animParams.skeleton = &m_playerAnimAssets->referenceSkeleton;

				animParams.jointOffset = params.mesh.vertexOffset;
				animParams.jointRotation = params.mesh.vertexRotation;
				animParams.jointScale = params.mesh.vertexScale;

				Nz::MaterialSettings settings;
				Nz::PredefinedMaterials::AddBasicSettings(settings);
				Nz::PredefinedMaterials::AddPbrSettings(settings);
				settings.AddTextureProperty("AmbientOcclusionMap", Nz::ImageType::E2D);
				settings.AddTextureProperty("MetalnessSmoothnessMap", Nz::ImageType::E2D);
				settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("AmbientOcclusionMap", "HasAmbientOcclusionTexture"));
				settings.AddPropertyHandler(std::make_unique<Nz::TexturePropertyHandler>("MetalnessSmoothnessMap", "HasMetalnessSmoothnessTexture"));

				Nz::MaterialPass forwardPass;
				forwardPass.states.depthBuffer = true;
				forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.PlayerPBR"));
				settings.AddPass("ForwardPass", forwardPass);

				Nz::MaterialPass depthPass = forwardPass;
				depthPass.options[nzsl::Ast::HashOption("DepthPass")] = true;
				settings.AddPass("DepthPass", depthPass);

				Nz::MaterialPass shadowPass = depthPass;
				shadowPass.options[nzsl::Ast::HashOption("ShadowPass")] = true;
				shadowPass.states.frontFace = Nz::FrontFace::Clockwise;
				shadowPass.states.depthClamp = Nz::Graphics::Instance()->GetRenderDevice()->GetEnabledFeatures().depthClamping;
				settings.AddPass("ShadowPass", shadowPass);

				Nz::MaterialPass distanceShadowPass = shadowPass;
				distanceShadowPass.options[nzsl::Ast::HashOption("DistanceDepth")] = true;
				settings.AddPass("DistanceShadowPass", distanceShadowPass);

				auto playerMaterial = std::make_shared<Nz::Material>(std::move(settings), "TSOM.PlayerPBR");

				std::shared_ptr<Nz::MaterialInstance> playerMat = playerMaterial->Instantiate();
				playerMat->SetTextureProperty("BaseColorMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_AlbedoTransparency.png", { .sRGB = true }));
				playerMat->SetTextureProperty("AmbientOcclusionMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_AO.png"));
				playerMat->SetTextureProperty("MetalnessSmoothnessMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_Normal.png"));
				playerMat->SetTextureProperty("NormalMap", fs.Open<Nz::TextureAsset>("assets/Player/Textures/Soldier_Normal.png"));

				m_playerModel->model->SetMaterial(0, std::move(playerMat));

				m_playerAnimAssets->idleAnimation = fs.Load<Nz::Animation>("assets/Player/Idle.fbx", animParams);
				m_playerAnimAssets->runningAnimation = fs.Load<Nz::Animation>("assets/Player/Running.fbx", animParams);
				m_playerAnimAssets->walkingAnimation = fs.Load<Nz::Animation>("assets/Player/Walking.fbx", animParams);
			}
			else
			{
				// Fallback
				std::shared_ptr<Nz::Mesh> mesh = Nz::Mesh::Build(collider->GenerateDebugMesh());

				std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
				colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
				colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
				{
					states.primitiveMode = Nz::PrimitiveMode::LineList;
					return true;
				});

				std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*mesh);

				m_playerModel->model = std::make_shared<Nz::Model>(colliderGraphicalMesh);
				for (std::size_t i = 0; i < m_playerModel->model->GetSubMeshCount(); ++i)
					m_playerModel->model->SetMaterial(i, colliderMat);
			}
		}

		auto& gfx = entity.emplace<Nz::GraphicsComponent>();
		gfx.AttachRenderable(m_playerModel->model, (entityData.controllingPlayerId == m_ownPlayerIndex) ? tsom::Constants::RenderMaskLocalPlayer : tsom::Constants::RenderMaskOtherPlayer);

		// Skeleton & animations
		std::shared_ptr<Nz::Skeleton> skeleton = std::make_shared<Nz::Skeleton>(m_playerAnimAssets->referenceSkeleton);

		auto& skeletonComponent = entity.emplace<Nz::SkeletonComponent>(skeleton);

		entity.emplace<AnimationComponent>(skeleton, std::make_shared<PlayerAnimationController>(entity, m_playerAnimAssets));

		// Floating name
		std::shared_ptr<Nz::TextSprite> textSprite = std::make_shared<Nz::TextSprite>();

		PlayerInfo* playerInfo = FetchPlayerInfo(entityData.controllingPlayerId);
		if (playerInfo)
			textSprite->Update(Nz::SimpleTextDrawer::Draw(playerInfo->nickname, 48, Nz::TextStyle_Regular, (playerInfo->isAuthenticated) ? Nz::Color::White() : Nz::Color::Gray()), 0.01f);
		else
			textSprite->Update(Nz::SimpleTextDrawer::Draw("<disconnected>", 48, Nz::TextStyle_Regular, Nz::Color::Gray()), 0.01f);

		entt::handle frontTextEntity = m_world.CreateEntity();
		{
			auto& textNode = frontTextEntity.emplace<Nz::NodeComponent>();
			textNode.SetParent(entity);
			textNode.SetPosition({ -textSprite->GetAABB().width * 0.5f, 1.5f, 0.f });

			frontTextEntity.emplace<Nz::GraphicsComponent>(textSprite);
		}
		entity.get_or_emplace<EntityOwnerComponent>().Register(frontTextEntity);

		entt::handle backTextEntity = m_world.CreateEntity();
		{
			auto& textNode = backTextEntity.emplace<Nz::NodeComponent>();
			textNode.SetParent(entity);
			textNode.SetPosition({ textSprite->GetAABB().width * 0.5f, 1.5f, 0.f });
			textNode.SetRotation(Nz::EulerAnglesf(0.f, Nz::TurnAnglef(0.5f), 0.f));

			backTextEntity.emplace<Nz::GraphicsComponent>(textSprite);
		}

		entity.get_or_emplace<EntityOwnerComponent>().Register(backTextEntity);

		if (entityData.controllingPlayerId == m_ownPlayerIndex)
		{
			m_playerControlledEntity = entity;
			OnControlledEntityChanged(entity);
		}
		else
			entity.emplace<MovementInterpolationComponent>(m_lastTickIndex);

		if (playerInfo)
			playerInfo->textSprite = std::move(textSprite);
	}

	void ClientSessionHandler::SetupEntity(entt::handle entity, Packets::Helper::ShipData&& entityData)
	{
		auto& shipComp = entity.emplace<ShipComponent>();
		shipComp.ship = std::make_unique<Ship>(m_blockLibrary, Nz::Vector3ui(32), 1.f);

		std::shared_ptr<Nz::Collider3D> chunkCollider = shipComp.ship->GetChunk(0)->BuildCollider(m_blockLibrary);

		entity.emplace<Nz::RigidBody3DComponent>(Nz::RigidBody3DComponent::DynamicSettings(chunkCollider, 100.f));

		// Fallback
		std::shared_ptr<Nz::Mesh> mesh = Nz::Mesh::Build(chunkCollider->GenerateDebugMesh());

		std::shared_ptr<Nz::MaterialInstance> colliderMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
		colliderMat->SetValueProperty("BaseColor", Nz::Color::Green());
		colliderMat->UpdatePassesStates([](Nz::RenderStates& states)
		{
			states.primitiveMode = Nz::PrimitiveMode::LineList;
			return true;
		});

		std::shared_ptr<Nz::GraphicalMesh> colliderGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*mesh);

		std::shared_ptr<Nz::Model> model = std::make_shared<Nz::Model>(colliderGraphicalMesh);
		for (std::size_t i = 0; i < model->GetSubMeshCount(); ++i)
			model->SetMaterial(i, colliderMat);

		auto& gfx = entity.emplace<Nz::GraphicsComponent>();
		gfx.AttachRenderable(model);
	}
}
