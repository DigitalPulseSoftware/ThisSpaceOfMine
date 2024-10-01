// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/PlayerAnimationController.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/AnimationComponent.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <ClientLib/Components/EnvironmentComponent.hpp>
#include <ClientLib/Components/MovementInterpolationComponent.hpp>
#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/Scripting/ClientEntityScriptingLibrary.hpp>
#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Scripting/SharedEntityScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Components/LifetimeComponent.hpp>
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
#include <Nazara/Physics3D/Components/PhysCharacter3DComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::AuthRequest>,        { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ExitShipControl>,    { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::Interact>,           { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
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
	m_currentEnvironmentIndex(Nz::MaxValue()),
	m_scriptingContext(app),
	m_lastInputIndex(0)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);

		m_scriptingContext.RegisterLibrary<MathScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<ClientScriptingLibrary>(m_app, *this);
		m_scriptingContext.LoadDirectory("scripts/assets");

		m_entityRegistry.RegisterClassLibrary<ClientChunkClassLibrary>(m_app, m_blockLibrary);

		m_scriptingContext.RegisterLibrary<ClientEntityScriptingLibrary>(m_entityRegistry);

		LoadScripts();
	}

	ClientSessionHandler::~ClientSessionHandler()
	{
		for (auto& entityDataOpt : m_entities)
		{
			if (entityDataOpt)
				entityDataOpt->entity.destroy();
		}
	}

	void ClientSessionHandler::EnableShipControl(bool enable)
	{
		OnShipControlUpdated(enable);
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
		ChunkIndices indices(chunkCreate.chunkLocX, chunkCreate.chunkLocY, chunkCreate.chunkLocZ);

		assert(m_entities[chunkCreate.entityId]);
		entt::handle& entity = m_entities[chunkCreate.entityId]->entity;

		Chunk* chunk;
		if (PlanetComponent* planetComponent = entity.try_get<PlanetComponent>())
			chunk = &planetComponent->planet->AddChunk(m_blockLibrary, indices);
		else if (ShipComponent* shipComponent = entity.try_get<ShipComponent>())
			chunk = &shipComponent->ship->AddChunk(m_blockLibrary, indices);

		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();
		chunkNetworkMap.chunkByNetworkIndex.emplace(chunkCreate.chunkId, chunk);
		chunkNetworkMap.chunkNetworkIndices.emplace(chunk, chunkCreate.chunkId);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkDestroy&& chunkDestroy)
	{
		assert(m_entities[chunkDestroy.entityId]);
		entt::handle& entity = m_entities[chunkDestroy.entityId]->entity;
		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

		auto it = chunkNetworkMap.chunkByNetworkIndex.find(chunkDestroy.chunkId);

		Chunk* chunk = it->second;
		chunk->GetContainer().RemoveChunk(chunk->GetIndices());

		chunkNetworkMap.chunkNetworkIndices.erase(chunk);
		chunkNetworkMap.chunkByNetworkIndex.erase(it);
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkReset&& chunkReset)
	{
		assert(m_entities[chunkReset.entityId]);
		entt::handle& entity = m_entities[chunkReset.entityId]->entity;
		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

		Chunk* chunk = Nz::Retrieve(chunkNetworkMap.chunkByNetworkIndex, chunkReset.chunkId);
		if (!chunk)
		{
			fmt::print(fg(fmt::color::red), "ChunkReset handler: unknown chunk {}\n", chunkReset.chunkId);
			return;
		}

		chunk->LockWrite();
		chunk->Reset([&](BlockIndex* blocks)
		{
			for (BlockIndex blockContent : chunkReset.content)
				*blocks++ = blockContent;
		});
		chunk->UnlockWrite();
	}

	void ClientSessionHandler::HandlePacket(Packets::ChunkUpdate&& chunkUpdate)
	{
		assert(m_entities[chunkUpdate.entityId]);
		entt::handle& entity = m_entities[chunkUpdate.entityId]->entity;
		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

		Chunk* chunk = Nz::Retrieve(chunkNetworkMap.chunkByNetworkIndex, chunkUpdate.chunkId);
		chunk->LockWrite();

		for (auto&& [blockPos, blockIndex] : chunkUpdate.updates)
			chunk->UpdateBlock({ blockPos.x, blockPos.y, blockPos.z }, Nz::SafeCast<BlockIndex>(blockIndex));

		chunk->UnlockWrite();
	}

	void ClientSessionHandler::HandlePacket(Packets::DebugDrawLineList&& debugDrawLineList)
	{
		std::shared_ptr<Nz::MaterialInstance> debugMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
		debugMat->SetValueProperty("BaseColor", debugDrawLineList.color);
		debugMat->UpdatePassesStates([](Nz::RenderStates& states)
		{
			states.depthBuffer = false;
			states.primitiveMode = Nz::PrimitiveMode::LineList;
			return true;
		});

		std::shared_ptr<Nz::VertexBuffer> debugVB = std::make_shared<Nz::VertexBuffer>(Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ), Nz::SafeCast<Nz::UInt32>(debugDrawLineList.vertices.size()), Nz::BufferUsage::Write, Nz::SoftwareBufferFactory, debugDrawLineList.vertices.data());
		std::shared_ptr<Nz::IndexBuffer> debugIB = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U16, Nz::SafeCast<Nz::UInt32>(debugDrawLineList.indices.size()), Nz::BufferUsage::Write, Nz::SoftwareBufferFactory, debugDrawLineList.indices.data());

		std::shared_ptr<Nz::StaticMesh> debugSubMesh = std::make_shared<Nz::StaticMesh>(std::move(debugVB), std::move(debugIB));
		debugSubMesh->GenerateAABB();
		debugSubMesh->SetPrimitiveMode(Nz::PrimitiveMode::LineList);

		std::shared_ptr<Nz::Mesh> debugMesh = Nz::Mesh::Build(std::move(debugSubMesh));
		std::shared_ptr<Nz::GraphicalMesh> debugGraphicalMesh = Nz::GraphicalMesh::BuildFromMesh(*debugMesh);

		std::shared_ptr<Nz::Model> debugModel = std::make_shared<Nz::Model>(debugGraphicalMesh);
		for (std::size_t i = 0; i < debugModel->GetSubMeshCount(); ++i)
			debugModel->SetMaterial(i, debugMat);

		entt::handle debugEntity = m_world.CreateEntity();

		assert(m_environments[debugDrawLineList.environmentId]);
		auto& environment = *m_environments[debugDrawLineList.environmentId];

		auto& entityNode = debugEntity.emplace<Nz::NodeComponent>(debugDrawLineList.position, debugDrawLineList.rotation);
		entityNode.SetParent(environment.rootNode);

		auto& gfxComponent = debugEntity.emplace<Nz::GraphicsComponent>();
		gfxComponent.AttachRenderable(std::move(debugModel), tsom::Constants::RenderMask3D);

		debugEntity.emplace<Nz::LifetimeComponent>(Nz::Time::Seconds(debugDrawLineList.duration));
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesCreation&& entitiesCreation)
	{
		for (auto&& entityData : entitiesCreation.entities)
		{
			entt::handle entity = m_world.CreateEntity();

			if (entityData.entityId >= m_entities.size())
				m_entities.resize(entityData.entityId + 1);

			assert(!m_entities[entityData.entityId]);
			m_entities[entityData.entityId] = EntityData{
				.environmentIndex = entityData.environmentId,
				.entity = entity
			};

			assert(m_environments[entityData.environmentId]);
			auto& environment = *m_environments[entityData.environmentId];
			environment.entities.UnboundedSet(entityData.entityId);

			auto& entityNode = entity.emplace<Nz::NodeComponent>(entityData.initialStates.position, entityData.initialStates.rotation);
			entityNode.SetParent(environment.rootNode);

			auto& entityEnv = entity.emplace<EnvironmentComponent>();
			entityEnv.environmentIndex = entityData.environmentId;

			auto& entityNetId = entity.emplace<ClientEntityNetworkIndex>();
			entityNetId.networkIndex = entityData.entityId;

			std::string entityClassName = GetSession()->GetStringStore().GetString(entityData.entityClass);
			if (std::shared_ptr<const EntityClass> entityClass = m_entityRegistry.FindClass(entityClassName))
			{
				auto& entityInstance = entity.emplace<ClassInstanceComponent>(entityClass);

				std::size_t networkedPropertyIndex = 0;
				for (Nz::UInt32 i = 0; i < entityClass->GetPropertyCount(); ++i)
				{
					if (entityClass->GetProperty(i).isNetworked)
						entityInstance.UpdateProperty(i, std::move(entityData.properties[networkedPropertyIndex++]));
				}

				entityClass->ActivateEntity(entity);
			}
			else
				fmt::print(fg(fmt::color::red), "unknown entity class {}\n", entityClassName);

			if (entityData.playerControlled)
				SetupEntity(entity, std::move(entityData.playerControlled.value()));

			// TEMP
			if (PlanetComponent* planetComponent = entity.try_get<PlanetComponent>())
				environment.gravityController = planetComponent->planet.get();
			else if (ShipComponent* shipComponent = entity.try_get<ShipComponent>())
				environment.gravityController = shipComponent->ship.get();

			fmt::print("Created entity {} in environment {} ({})\n", entityData.entityId, entityData.environmentId, entityClassName);

			// Since we make use of parenting for environments, we need to make replication happen in global space
			if (Nz::RigidBody3DComponent* rigidBody = entity.try_get<Nz::RigidBody3DComponent>())
			{
				if (rigidBody->GetReplicationMode() == Nz::PhysicsReplication3D::Local)
					rigidBody->SetReplicationMode(Nz::PhysicsReplication3D::Global);
			}
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesDelete&& entitiesDelete)
	{
		for (auto entityId : entitiesDelete.entities)
		{
			assert(m_entities[entityId]);
			EntityData& entityData = *m_entities[entityId];
			Packets::Helper::EnvironmentId environmentIndex = entityData.environmentIndex;
			assert(m_environments[environmentIndex]);
			EnvironmentData& environmentData = *m_environments[environmentIndex];
			environmentData.entities.Reset(entityId);

			if (m_playerControlledEntity == entityData.entity)
				OnControlledEntityChanged({});

			entityData.entity.destroy();
			m_entities[entityId].reset();
			fmt::print("Deleted entity {} from environment {}\n", entityId, environmentIndex);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::EntitiesStateUpdate&& stateUpdate)
	{
		for (auto& entityStates : stateUpdate.entities)
		{
			assert(m_entities[entityStates.entityId]);
			EntityData& entityData = *m_entities[entityStates.entityId];

			if (MovementInterpolationComponent* movementInterpolation = entityData.entity.try_get<MovementInterpolationComponent>())
				movementInterpolation->PushMovement(stateUpdate.tickIndex, entityStates.newStates.position, entityStates.newStates.rotation);
			else if (Nz::RigidBody3DComponent* rigidBody = entityData.entity.try_get<Nz::RigidBody3DComponent>())
			{
				// physics is in global space
				EnvironmentData& envData = *m_environments[entityData.environmentIndex];
				Nz::Vector3f globalPos = envData.rootNode.ToGlobalPosition(entityStates.newStates.position);
				Nz::Quaternionf globalRot = envData.rootNode.ToGlobalRotation(entityStates.newStates.rotation);

				rigidBody->TeleportTo(globalPos, globalRot);
			}
			else
			{
				auto& entityNode = entityData.entity.get<Nz::NodeComponent>();
				entityNode.SetTransform(entityStates.newStates.position, entityStates.newStates.rotation);
			}
		}

		if (stateUpdate.controlledCharacter)
			OnControlledEntityStateUpdate(stateUpdate.lastInputIndex, *stateUpdate.controlledCharacter);
	}

	void ClientSessionHandler::HandlePacket(Packets::EntityEnvironmentUpdate&& environmentUpdate)
	{
		assert(m_entities[environmentUpdate.entity]);
		EntityData& entityData = *m_entities[environmentUpdate.entity];
		fmt::print("Entity {} moved to environment #{} to environment #{}\n", environmentUpdate.entity, entityData.environmentIndex, environmentUpdate.newEnvironmentId);

		assert(m_environments[entityData.environmentIndex]);
		auto& oldEnvironment = *m_environments[entityData.environmentIndex];
		oldEnvironment.entities.Reset(environmentUpdate.entity);

		assert(m_environments[environmentUpdate.newEnvironmentId]);
		auto& newEnvironment = *m_environments[environmentUpdate.newEnvironmentId];
		newEnvironment.entities.UnboundedSet(environmentUpdate.entity);

		auto& entityNode = entityData.entity.get<Nz::NodeComponent>();
		entityNode.SetParent(newEnvironment.rootNode, true);

		auto& entityEnv = entityData.entity.get<EnvironmentComponent>();
		entityEnv.environmentIndex = environmentUpdate.newEnvironmentId;

		entityData.environmentIndex = environmentUpdate.newEnvironmentId;
		if (MovementInterpolationComponent* movementInterpolation = entityData.entity.try_get<MovementInterpolationComponent>())
			movementInterpolation->UpdateRoot(oldEnvironment.rootNode, newEnvironment.rootNode);
	}

	void ClientSessionHandler::HandlePacket(Packets::EntityProcedureCall&& procedureCall)
	{
		assert(m_entities[procedureCall.entity]);
		EntityData& entityData = *m_entities[procedureCall.entity];

		auto& classInstance = entityData.entity.get<ClassInstanceComponent>();
		const auto& clientRpc = classInstance.GetClass()->GetClientRpc(procedureCall.rpcIndex);
		if (clientRpc.onCalled)
			clientRpc.onCalled(entityData.entity);
		else
			fmt::print(fg(fmt::color::yellow), "client rpc {} has been triggered but has no callback\n", clientRpc.name);
	}

	void ClientSessionHandler::HandlePacket(Packets::EntityPropertyUpdate&& propertyUpdate)
	{
		assert(m_entities[propertyUpdate.entity]);
		EntityData& entityData = *m_entities[propertyUpdate.entity];

		auto& classInstance = entityData.entity.get<ClassInstanceComponent>();
		classInstance.UpdateProperty(propertyUpdate.propertyIndex, std::move(propertyUpdate.propertyValue));
	}

	void ClientSessionHandler::HandlePacket(Packets::EnvironmentCreate&& envCreate)
	{
		fmt::print("Created environment #{} at {};{};{}\n", envCreate.id, envCreate.transform.translation.x, envCreate.transform.translation.y, envCreate.transform.translation.z);
		if (envCreate.id >= m_environments.size())
			m_environments.resize(envCreate.id + 1);

		auto& environment = m_environments[envCreate.id].emplace();
		environment.transform = envCreate.transform;
		environment.rootNode.SetTransform(envCreate.transform.translation, envCreate.transform.rotation);
	}

	void ClientSessionHandler::HandlePacket(Packets::EnvironmentDestroy&& envDestroy)
	{
		fmt::print("Destroyed environment #{}\n", envDestroy.id);
		for (std::size_t entityIndex : m_environments[envDestroy.id]->entities.IterBits())
		{
			assert(m_entities[entityIndex]);
			EntityData& entityData = *m_entities[entityIndex];
			entityData.entity.destroy();

			m_entities[entityIndex].reset();
		}

		m_environments[envDestroy.id].reset();
	}

	void ClientSessionHandler::HandlePacket(Packets::EnvironmentUpdate&& envUpdate)
	{
		assert(m_environments[envUpdate.id]);
		auto& environmentData = *m_environments[envUpdate.id];
		environmentData.transform = envUpdate.transform;
		environmentData.rootNode.SetTransform(environmentData.transform.translation, environmentData.transform.rotation);

		// Teleport physical entities
		for (std::size_t entityIndex : environmentData.entities.IterBits())
		{
			assert(m_entities[entityIndex]);
			EntityData& entityData = *m_entities[entityIndex];

			Nz::NodeComponent& entityNode = entityData.entity.get<Nz::NodeComponent>();

			if (Nz::RigidBody3DComponent* rigidBody = entityData.entity.try_get<Nz::RigidBody3DComponent>())
			{
				rigidBody->TeleportTo(entityNode.GetGlobalPosition(), entityNode.GetGlobalRotation());
				if (rigidBody->IsStatic())
					rigidBody->SetReplicationMode(Nz::PhysicsReplication3D::GlobalOnce);
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
			playerInfo.nickname = std::move(playerData.nickname).Str();
			playerInfo.isAuthenticated = playerData.isAuthenticated;
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::NetworkStrings&& networkStrings)
	{
		GetSession()->GetStringStore().FillStore(networkStrings.startId, std::move(networkStrings.strings));
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

	void ClientSessionHandler::HandlePacket(Packets::UpdateRootEnvironment&& playerEnv)
	{
		if (m_currentEnvironmentIndex != std::numeric_limits<Packets::Helper::EnvironmentId>::max())
		{
			assert(m_environments[playerEnv.newRootEnv]);
			EnvironmentTransform inverseTransform = -m_environments[playerEnv.newRootEnv]->transform;

			for (auto& envOpt : m_environments)
			{
				if (!envOpt)
					continue;

				auto& env = *envOpt;
				env.transform += inverseTransform;

				env.rootNode.SetTransform(env.transform.translation, env.transform.rotation);

				// Teleport physical entities
				for (std::size_t entityIndex : env.entities.IterBits())
				{
					assert(m_entities[entityIndex]);
					EntityData& entityData = *m_entities[entityIndex];

					Nz::NodeComponent& entityNode = entityData.entity.get<Nz::NodeComponent>();

					if (Nz::RigidBody3DComponent* rigidBody = entityData.entity.try_get<Nz::RigidBody3DComponent>())
						rigidBody->TeleportTo(entityNode.GetGlobalPosition(), entityNode.GetGlobalRotation());
				}
			}
		}

		m_currentEnvironmentIndex = playerEnv.newRootEnv;
	}

	void ClientSessionHandler::LoadScripts(bool isReloading)
	{
		if (!isReloading)
		{
			m_scriptingContext.LoadDirectory("scripts/entities");
			return;
		}

		entt::registry* reg = &m_world.GetRegistry();
		m_entityRegistry.Refresh(std::span(&reg, 1), [this]
		{
			LoadScripts(false);
		});
	}

	void ClientSessionHandler::SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData)
	{
		auto collider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);

		Nz::RigidBody3D::DynamicSettings physSettings(collider, 0.f);
		physSettings.objectLayer = Constants::ObjectLayerPlayer;

		entity.emplace<Nz::RigidBody3DComponent>(physSettings, Nz::PhysicsReplication3D::None);

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
}
