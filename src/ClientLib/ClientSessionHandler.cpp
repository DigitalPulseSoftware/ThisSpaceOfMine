// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/PlayerAnimationController.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/AnimationComponent.hpp>
#include <ClientLib/Components/CameraFollowerComponent.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <ClientLib/Components/EnvironmentComponent.hpp>
#include <ClientLib/Components/NetworkInterpolationComponent.hpp>
#include <ClientLib/Components/PhysicsInterpolationComponent.hpp>
#include <ClientLib/Components/TransformCopyComponent.hpp>
#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/Entities/ClientEntityClassLibrary.hpp>
#include <ClientLib/Scripting/ClientAssetScriptingLibrary.hpp>
#include <ClientLib/Scripting/ClientEntityScriptingLibrary.hpp>
#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Scripting/BaseScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/SharedEntityScriptingLibrary.hpp>
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
#include <spdlog/spdlog.h>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::C_AuthRequest>,        { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_ConnectEntities>,    { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_ExitShipControl>,    { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_Interact>,           { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_GrabEntity>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_MineBlock>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_PlaceBlock>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_PlaceEntity>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_RemoveEntity>,       { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_SendChatMessage>,    { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_SendConsoleCommand>, { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::C_UpdatePlayerInputs>, { .channel = 1, .flags = Nz::ENetPacketFlag_Unreliable } }
	});

	ClientSessionHandler::ClientSessionHandler(NetworkSession* session, Nz::ApplicationBase& app, ConfigFile& config, Nz::EnttWorld& world, ClientBlockLibrary& blockLibrary) :
	SessionHandler(session),
	m_app(app),
	m_world(world),
	m_blockLibrary(blockLibrary),
	m_config(config),
	m_ownPlayerIndex(InvalidPlayerIndex),
	m_scriptingContext(app),
	m_lastInputIndex(0)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);

		m_scriptingContext.RegisterLibrary<BaseScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<MathScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<ClientAssetScriptingLibrary>(m_app);
		m_scriptingContext.LoadDirectory("scripts/libraries");
		m_scriptingContext.LoadDirectory("scripts/assets");

		m_entityRegistry.RegisterClassLibrary<ClientChunkClassLibrary>(m_app, config, m_blockLibrary);
		m_entityRegistry.RegisterClassLibrary<ClientEntityClassLibrary>(m_app);

		m_scriptingContext.RegisterLibrary<ClientEntityScriptingLibrary>(m_entityRegistry);
		m_scriptingContext.RegisterLibrary<ClientScriptingLibrary>(m_app, config, *this);

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

	inline EntityRegistry& ClientSessionHandler::GetEntityRegistry()
	{
		return m_entityRegistry;
	}

	inline const EntityRegistry& ClientSessionHandler::GetEntityRegistry() const
	{
		return m_entityRegistry;
	}

	const Nz::Node* ClientSessionHandler::GetEnvironmentNode(std::size_t environmentIndex) const
	{
		if (environmentIndex > m_environments.size() || !m_environments[environmentIndex])
			return nullptr;

		return m_environments[environmentIndex]->rootEntity.try_get<Nz::NodeComponent>();
	}

	void ClientSessionHandler::HandlePacket(Packets::S_AuthResponse&& authResponse)
	{
		if (authResponse.authResult.IsOk())
			m_ownPlayerIndex = authResponse.ownPlayerIndex;

		OnAuthResponse(authResponse);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChatMessage&& chatMessage)
	{
		if (chatMessage.playerIndex)
		{
			if (chatMessage.playerIndex >= m_players.size())
			{
				spdlog::error("ChatMessage with unknown player index {}", *chatMessage.playerIndex);
				return;
			}

			OnPlayerChatMessage(chatMessage.message, *m_players[*chatMessage.playerIndex]);
		}
		else
			OnChatMessage(chatMessage.message);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkCreate&& chunkCreate)
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

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkDestroy&& chunkDestroy)
	{
		assert(m_entities[chunkDestroy.entityId]);
		entt::handle& entity = m_entities[chunkDestroy.entityId]->entity;
		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

		auto it = chunkNetworkMap.chunkByNetworkIndex.find(chunkDestroy.chunkId);

		Chunk* chunk = it->second;
		chunk->GetContainer().RemoveChunk(chunk->GetIndices());

		chunkNetworkMap.chunkNetworkIndices.erase(chunk);
		chunkNetworkMap.chunkByNetworkIndex.erase(it);

		m_pendingChunkReset.erase(chunkDestroy.chunkId);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkReset&& chunkReset)
	{
		assert(m_entities[chunkReset.entityId]);
		entt::handle& entity = m_entities[chunkReset.entityId]->entity;
		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

		Chunk* chunk = Nz::Retrieve(chunkNetworkMap.chunkByNetworkIndex, chunkReset.chunkId);
		if (!chunk)
		{
			spdlog::error("ChunkReset handler: unknown chunk {}", chunkReset.chunkId);
			return;
		}

		ChunkWriteLock lock(chunk, std::defer_lock);

		if (lock.TryLock())
		{
			if (!chunkReset.content.empty())
			{
				chunk->Reset([&](BlockIndex* blocks)
				{
					for (BlockIndex blockContent : chunkReset.content)
						*blocks++ = blockContent;
				});
			}
			else
				chunk->ClearContent();
		}
		else
			m_pendingChunkReset[chunkReset.chunkId] = std::move(chunkReset);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ChunkUpdate&& chunkUpdate)
	{
		assert(m_entities[chunkUpdate.entityId]);
		entt::handle& entity = m_entities[chunkUpdate.entityId]->entity;
		auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

		Chunk* chunk = Nz::Retrieve(chunkNetworkMap.chunkByNetworkIndex, chunkUpdate.chunkId);

		if (auto it = m_pendingChunkReset.find(chunkUpdate.chunkId); it != m_pendingChunkReset.end())
		{
			// Apply update to pending chunk reset
			Packets::S_ChunkReset& pendingChunkReset = it.value();
			if (pendingChunkReset.content.empty())
				pendingChunkReset.content.resize(chunk->GetBlockCount(), EmptyBlockIndex);

			for (auto&& [blockPos, blockIndex] : chunkUpdate.updates)
				pendingChunkReset.content[chunk->GetBlockLocalIndex({ blockPos.x, blockPos.y, blockPos.z })] = Nz::SafeCast<BlockIndex>(blockIndex);
		}
		else
		{
			ChunkWriteLock lock(chunk);

			for (auto&& [blockPos, blockIndex] : chunkUpdate.updates)
				chunk->UpdateBlock({ blockPos.x, blockPos.y, blockPos.z }, Nz::SafeCast<BlockIndex>(blockIndex));
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::S_ConsoleOutput&& consoleOutput)
	{
		OnConsoleOutput(consoleOutput.color, consoleOutput.output);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_DebugDrawLineList&& debugDrawLineList)
	{
		OnDebugDrawLineList(debugDrawLineList);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntitiesCreation&& entitiesCreation)
	{
		for (auto&& entityData : entitiesCreation.entities)
			HandleEntityCreation(std::move(entityData));
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntitiesDelete&& entitiesDelete)
	{
		for (auto entityId : entitiesDelete.entities)
		{
			NazaraAssert(entityId < m_entities.size() && m_entities[entityId]);
			EntityData& entityData = *m_entities[entityId];
			Packets::Helper::EnvironmentId environmentIndex = entityData.environmentIndex;
			assert(m_environments[environmentIndex]);
			EnvironmentData& environmentData = *m_environments[environmentIndex];
			environmentData.entities.Reset(entityId);

			if (m_playerControlledEntity == entityData.entity)
				OnControlledEntityChanged({});

			entityData.entity.destroy();
			m_entities[entityId].reset();
			spdlog::info("Deleted entity {} from environment {}", entityId, environmentIndex);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntitiesStateUpdate&& stateUpdate)
	{
		for (auto& entityStates : stateUpdate.entities)
		{
			NazaraAssert(entityStates.entityId < m_entities.size() && m_entities[entityStates.entityId]);
			EntityData& entityData = *m_entities[entityStates.entityId];

			if (NetworkInterpolationComponent* movementInterpolation = entityData.entity.try_get<NetworkInterpolationComponent>())
				movementInterpolation->PushMovement(stateUpdate.tickIndex, entityStates.newStates.position, entityStates.newStates.rotation);
			else if (Nz::RigidBody3DComponent* rigidBody = entityData.entity.try_get<Nz::RigidBody3DComponent>())
			{
				// physics is in global space
				EnvironmentData& envData = *m_environments[entityData.environmentIndex];
				auto& rootNode = envData.rootEntity.get<Nz::NodeComponent>();
				Nz::Vector3f globalPos = rootNode.ToGlobalPosition(entityStates.newStates.position);
				Nz::Quaternionf globalRot = rootNode.ToGlobalRotation(entityStates.newStates.rotation);

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

	void ClientSessionHandler::HandlePacket(Packets::S_EntityDistributionUpdate&& distributionUpdate)
	{
		NazaraAssert(distributionUpdate.sourceEntity < m_entities.size() && m_entities[distributionUpdate.sourceEntity]);
		NazaraAssert(distributionUpdate.targetEntity < m_entities.size() && m_entities[distributionUpdate.targetEntity]);
		EntityData& sourceEntityData = *m_entities[distributionUpdate.sourceEntity];
		EntityData& targetEntityData = *m_entities[distributionUpdate.targetEntity];

		auto* sourceEntityDistribution = sourceEntityData.entity.try_get<DistributionComponent>();
		if (!sourceEntityDistribution)
		{
			spdlog::error("Received a connection update for entity {} which has no distribution component (is the entity correctly initialized?)", distributionUpdate.sourceEntity);
			return;
		}

		if (distributionUpdate.sourceEntityPort >= sourceEntityDistribution->GetOutputCount())
		{
			spdlog::error("Received a connection update for entity {} on output port {} but it only got {} port(s) (is the entity correctly initialized?)", distributionUpdate.sourceEntity, distributionUpdate.sourceEntityPort, sourceEntityDistribution->GetOutputCount());
			return;
		}

		auto* targetEntityDistribution = targetEntityData.entity.try_get<DistributionComponent>();
		if (!targetEntityDistribution)
		{
			spdlog::error("Received a connection update targeting entity {} which has no distribution component (is the entity correctly initialized?)", distributionUpdate.targetEntity);
			return;
		}

		if (distributionUpdate.targetEntityPort >= targetEntityDistribution->GetInputCount())
		{
			spdlog::error("Received a connection update targeting entity {} on input port {} but it only got {} port(s) (is the entity correctly initialized?)", distributionUpdate.targetEntity, distributionUpdate.targetEntityPort, sourceEntityDistribution->GetInputCount());
			return;
		}

		DistributionComponent::Connect(sourceEntityData.entity, *sourceEntityDistribution, targetEntityData.entity, *targetEntityDistribution, distributionUpdate.sourceEntityPort, distributionUpdate.targetEntityPort);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntityEnvironmentUpdate&& environmentUpdate)
	{
		NazaraAssert(environmentUpdate.entity < m_entities.size() && m_entities[environmentUpdate.entity]);
		EntityData& entityData = *m_entities[environmentUpdate.entity];
		spdlog::info("Entity {} moved to environment #{} to environment #{}", environmentUpdate.entity, entityData.environmentIndex, environmentUpdate.newEnvironmentId);

		NazaraAssert(entityData.environmentIndex < m_environments.size() && m_environments[entityData.environmentIndex]);
		auto& oldEnvironment = *m_environments[entityData.environmentIndex];
		oldEnvironment.entities.Reset(environmentUpdate.entity);

		NazaraAssert(environmentUpdate.newEnvironmentId < m_environments.size() && m_environments[environmentUpdate.newEnvironmentId]);
		auto& newEnvironment = *m_environments[environmentUpdate.newEnvironmentId];
		newEnvironment.entities.UnboundedSet(environmentUpdate.entity);

		auto& entityNode = entityData.entity.get<Nz::NodeComponent>();
		entityNode.SetParent(newEnvironment.rootEntity, true);

		auto& entityEnv = entityData.entity.get<EnvironmentComponent>();
		entityEnv.environmentIndex = environmentUpdate.newEnvironmentId;

		entityData.environmentIndex = environmentUpdate.newEnvironmentId;
		if (NetworkInterpolationComponent* movementInterpolation = entityData.entity.try_get<NetworkInterpolationComponent>())
			movementInterpolation->UpdateRoot(oldEnvironment.rootEntity.get<Nz::NodeComponent>(), newEnvironment.rootEntity.get<Nz::NodeComponent>());
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntityProcedureCall&& procedureCall)
	{
		NazaraAssert(procedureCall.entity < m_entities.size() && m_entities[procedureCall.entity]);
		EntityData& entityData = *m_entities[procedureCall.entity];

		auto& classInstance = entityData.entity.get<ClassInstanceComponent>();
		const auto& clientRpc = classInstance.GetClass()->GetClientRpc(procedureCall.rpcIndex);
		if (clientRpc.onCalled)
			clientRpc.onCalled(entityData.entity);
		else
			spdlog::warn("client rpc {} has been triggered but has no callback", clientRpc.name);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EntityPropertiesUpdate&& propertyUpdate)
	{
		assert(m_entities[propertyUpdate.entity]);
		EntityData& entityData = *m_entities[propertyUpdate.entity];

		auto& classInstance = entityData.entity.get<ClassInstanceComponent>();
		for (auto& propertyData : propertyUpdate.properties)
			classInstance.UpdateProperty(propertyData.index, std::move(propertyData.value));
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EnvironmentCreate&& envCreate)
	{
		spdlog::info("Created environment #{} (owned by {})", envCreate.id, envCreate.ownerEntity);
		if (envCreate.id >= m_environments.size())
			m_environments.resize(envCreate.id + 1);

		auto& environment = m_environments[envCreate.id].emplace();

		environment.rootEntity = m_world.CreateEntity();
		environment.rootEntity.emplace<Nz::NodeComponent>();

		if (envCreate.ownerEntity != Nz::MaxValue<Packets::Helper::EntityId>())
		{
			assert(m_entities[envCreate.ownerEntity]);
			EntityData& ownerEntity = *m_entities[envCreate.ownerEntity];

			auto& ownerNode = ownerEntity.entity.get<Nz::NodeComponent>();

			environment.rootEntity.get<Nz::NodeComponent>().SetParent(ownerNode);
		}

		for (auto&& entityData : envCreate.entities)
			HandleEntityCreation(std::move(entityData));
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EnvironmentDestroy&& envDestroy)
	{
		spdlog::info("Destroyed environment #{}", envDestroy.id);
		for (std::size_t entityIndex : m_environments[envDestroy.id]->entities.IterBits())
		{
			assert(m_entities[entityIndex]);
			EntityData& entityData = *m_entities[entityIndex];
			entityData.entity.destroy();

			m_entities[entityIndex].reset();
		}

		m_environments[envDestroy.id].reset();
	}

	void ClientSessionHandler::HandlePacket(Packets::S_EnvironmentsUpdateOwner&& envOwnerUpdate)
	{
		for (const auto& ownerUpdate : envOwnerUpdate.ownerUpdates)
		{
			spdlog::info("Environment #{} changed owned to {}", ownerUpdate.environment, ownerUpdate.newOwner);

			Nz::Node* ownerNode = nullptr;
			if (ownerUpdate.newOwner != Nz::MaxValue<Packets::Helper::EntityId>())
			{
				NazaraAssert(m_entities[ownerUpdate.newOwner]);
				EntityData& ownerEntity = *m_entities[ownerUpdate.newOwner];
				ownerNode = &ownerEntity.entity.get<Nz::NodeComponent>();
			}

			NazaraAssert(ownerUpdate.environment < m_environments.size() && m_environments[ownerUpdate.environment]);
			auto& envData = *m_environments[ownerUpdate.environment];
			envData.rootEntity.get<Nz::NodeComponent>().SetParent(ownerNode);
		}
	}

	void ClientSessionHandler::HandlePacket(Packets::S_GameData&& gameData)
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

	void ClientSessionHandler::HandlePacket(Packets::S_NetworkStrings&& networkStrings)
	{
		GetSession()->GetStringStore().FillStore(networkStrings.startId, std::move(networkStrings.strings));
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PilotShip&& pilotShip)
	{
		assert(m_entities[pilotShip.shipEntity]);
		entt::handle& entity = m_entities[pilotShip.shipEntity]->entity;
		entt::handle& exteriorEntity = m_entities[pilotShip.shipExteriorEntity]->entity;

		OnControlledShip(entity, exteriorEntity, pilotShip.referenceRotation);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PilotShipFinish&& pilotShipFinish)
	{
		OnControlledShipFinished();
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlayerJoin&& playerJoin)
	{
		if (playerJoin.index >= m_players.size())
			m_players.resize(playerJoin.index + 1);

		auto& playerInfo = m_players[playerJoin.index].emplace();
		playerInfo.nickname = std::move(playerJoin.nickname).Str();
		playerInfo.isAuthenticated = playerJoin.isAuthenticated;

		OnPlayerJoined(playerInfo);
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlayerLeave&& playerLeave)
	{
		if (playerLeave.index >= m_players.size() || !m_players[playerLeave.index])
		{
			spdlog::error("PlayerLeave with unknown player index {}", playerLeave.index);
			return;
		}

		OnPlayerLeave(*m_players[playerLeave.index]);

		m_players[playerLeave.index].reset();
	}

	void ClientSessionHandler::HandlePacket(Packets::S_PlayerNameUpdate&& playerNameUpdate)
	{
		if (playerNameUpdate.index >= m_players.size() || !m_players[playerNameUpdate.index])
		{
			spdlog::error("PlayerNameUpdate with unknown player index {}", playerNameUpdate.index);
			return;
		}

		auto& playerInfo = *m_players[playerNameUpdate.index];

		OnPlayerNameUpdate(playerInfo, playerNameUpdate.newNickname);
		playerInfo.nickname = std::move(playerNameUpdate.newNickname).Str();
		if (playerInfo.textSprite)
			playerInfo.textSprite->Update(Nz::SimpleTextDrawer::Draw(playerInfo.nickname, 48, Nz::TextStyle_Regular, (playerInfo.isAuthenticated) ? Nz::Color::White() : Nz::Color::Gray()), 0.01f);
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

	void ClientSessionHandler::Update()
	{
		for (auto it = m_pendingChunkReset.begin(); it != m_pendingChunkReset.end();)
		{
			Packets::Helper::ChunkId chunkId = it.key();
			const Packets::S_ChunkReset& chunkReset = it.value();

			assert(m_entities[chunkReset.entityId]);
			entt::handle& entity = m_entities[chunkReset.entityId]->entity;
			auto& chunkNetworkMap = entity.get<ChunkNetworkMapComponent>();

			Chunk* chunk = Nz::Retrieve(chunkNetworkMap.chunkByNetworkIndex, chunkReset.chunkId);
			if (!chunk)
			{
				spdlog::error("ChunkReset handler (pending): unknown chunk {}", chunkReset.chunkId);
				it = m_pendingChunkReset.erase(it);
				continue;
			}

			ChunkWriteLock lock(chunk, std::defer_lock);

			if (!lock.TryLock())
			{
				++it;
				continue;
			}

			if (!chunkReset.content.empty())
			{
				chunk->Reset([&](BlockIndex* blocks)
				{
					for (BlockIndex blockContent : chunkReset.content)
						*blocks++ = blockContent;
				});
			}
			else
				chunk->ClearContent();

			it = m_pendingChunkReset.erase(it);
		}
	}

	void ClientSessionHandler::HandleEntityCreation(Packets::Helper::EntityData&& entityData)
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

		// Create logical entity
		auto& entityNode = entity.emplace<Nz::NodeComponent>(entityData.initialStates.position, entityData.initialStates.rotation);
		entityNode.SetParent(environment.rootEntity);

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

			entityClass->InitAndActivateEntity(entity);
		}
		else
			spdlog::error("unknown entity class {}", entityClassName);

		if (entityData.playerControlled)
			SetupEntity(entity, std::move(entityData.playerControlled.value()));

		// TEMP
		if (PlanetComponent* planetComponent = entity.try_get<PlanetComponent>())
			environment.gravityController = planetComponent->planet.get();
		else if (ShipComponent* shipComponent = entity.try_get<ShipComponent>())
			environment.gravityController = shipComponent->ship.get();

		spdlog::info("Created entity {} in environment {} ({})", entityData.entityId, entityData.environmentId, entityClassName);

		// Since we make use of parenting for environments, we need to make replication happen in global space
		if (Nz::RigidBody3DComponent* rigidBody = entity.try_get<Nz::RigidBody3DComponent>())
		{
			switch (rigidBody->GetReplicationMode())
			{
				case Nz::PhysicsReplication3D::Local:
					rigidBody->SetReplicationMode(Nz::PhysicsReplication3D::Global);
					break;

				case Nz::PhysicsReplication3D::LocalOnce:
					rigidBody->SetReplicationMode(Nz::PhysicsReplication3D::GlobalOnce);
					break;

				case Nz::PhysicsReplication3D::Custom:
				case Nz::PhysicsReplication3D::CustomOnce:
				case Nz::PhysicsReplication3D::Global:
				case Nz::PhysicsReplication3D::GlobalOnce:
				case Nz::PhysicsReplication3D::None:
					break;
			}
		}
	}

	void ClientSessionHandler::SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData&& entityData)
	{
		auto collider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);

		Nz::RigidBody3D::DynamicSettings physSettings(collider, 0.f);
		physSettings.objectLayer = Constants::ObjectLayerPlayer;

		entity.emplace<Nz::RigidBody3DComponent>(physSettings, Nz::PhysicsReplication3D::None);

		// Player model
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

			m_playerModel->model = fs.Load<Nz::Model>("CookedAssets/Models/Player/Idle.fbx", params);
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

				auto& renderQueueRegistry = Nz::Graphics::Instance()->GetRenderQueueRegistry();
				std::size_t depthQueue = renderQueueRegistry.GetIndex("DepthOpaque");
				std::size_t forwardOpaqueQueue = renderQueueRegistry.GetIndex("ForwardOpaque");
				std::size_t forwardTransparentQueue = renderQueueRegistry.GetIndex("ForwardTransparent");
				std::size_t shadowQueue = renderQueueRegistry.GetIndex("Shadow");

				Nz::MaterialPass forwardPass;
				forwardPass.renderQueue = forwardOpaqueQueue;
				forwardPass.states.depthBuffer = true;
				forwardPass.states.depthCompare = Nz::RendererComparison::GreaterOrEqual;
				forwardPass.shaders.push_back(std::make_shared<Nz::UberShader>(nzsl::ShaderStageType::Fragment | nzsl::ShaderStageType::Vertex, "TSOM.PlayerPBR"));
				settings.AddPass("ForwardPass", forwardPass);

				Nz::MaterialPass depthPass = forwardPass;
				depthPass.renderQueue = depthQueue;
				depthPass.options[nzsl::Ast::HashOption("DepthPass")] = true;
				settings.AddPass("DepthPass", depthPass);

				Nz::MaterialPass shadowPass = depthPass;
				shadowPass.renderQueue = shadowQueue;
				shadowPass.options[nzsl::Ast::HashOption("ShadowPass")] = true;
				shadowPass.states.depthCompare = Nz::RendererComparison::LessOrEqual; //< TODO: Reverse depth for shadow pass?
				shadowPass.states.frontFace = Nz::FrontFace::Clockwise;
				shadowPass.states.depthClamp = Nz::Graphics::Instance()->GetGpuDevice()->GetEnabledFeatures().depthClamping;
				settings.AddPass("ShadowPass", shadowPass);

				auto playerMaterial = std::make_shared<Nz::Material>(std::move(settings), "TSOM.PlayerPBR");

				std::shared_ptr<Nz::MaterialInstance> playerMat = playerMaterial->Instantiate();
				playerMat->SetTextureProperty("BaseColorMap", fs.Open<Nz::TextureAsset>("CookedAssets/Models/Player/Textures/Soldier_AlbedoTransparency.dds", { .sRGB = true }));
				playerMat->SetTextureProperty("AmbientOcclusionMap", fs.Open<Nz::TextureAsset>("CookedAssets/Models/Player/Textures/Soldier_AO.dds"));
				playerMat->SetTextureProperty("MetalnessSmoothnessMap", fs.Open<Nz::TextureAsset>("CookedAssets/Models/Player/Textures/Soldier_MetallicSmoothness.dds"));
				playerMat->SetTextureProperty("NormalMap", fs.Open<Nz::TextureAsset>("CookedAssets/Models/Player/Textures/Soldier_Normal.dds"));

				m_playerModel->model->SetMaterial(0, std::move(playerMat));

				m_playerAnimAssets->idleAnimation = fs.Load<Nz::Animation>("CookedAssets/Models/Player/Idle.fbx", animParams);
				m_playerAnimAssets->runningAnimation = fs.Load<Nz::Animation>("CookedAssets/Models/Player/Running.fbx", animParams);
				m_playerAnimAssets->walkingAnimation = fs.Load<Nz::Animation>("CookedAssets/Models/Player/Walking.fbx", animParams);
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

		Nz::UInt32 playerRenderMask = (entityData.controllingPlayerId == m_ownPlayerIndex) ? tsom::Constants::RenderMaskLocalPlayer : tsom::Constants::RenderMaskOtherPlayer;

		auto& gfx = entity.emplace<Nz::GraphicsComponent>();
		gfx.AttachRenderable(m_playerModel->model, playerRenderMask);

		// Skeleton & animations
		std::shared_ptr<Nz::Skeleton> skeleton = std::make_shared<Nz::Skeleton>(m_playerAnimAssets->referenceSkeleton);

		auto& skeletonComponent = entity.emplace<Nz::SkeletonComponent>(skeleton);

		entity.emplace<AnimationComponent>(skeleton, std::make_shared<PlayerAnimationController>(entity, m_playerAnimAssets));

		// Floating name
		std::shared_ptr<Nz::TextSprite> textSprite = std::make_shared<Nz::TextSprite>(Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic, Nz::MaterialInstancePreset::ReverseZ | Nz::MaterialInstancePreset::AlphaBlended));

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

			frontTextEntity.emplace<Nz::GraphicsComponent>(textSprite, playerRenderMask);
		}
		entity.get_or_emplace<EntityOwnerComponent>().Register(frontTextEntity);

		entt::handle backTextEntity = m_world.CreateEntity();
		{
			auto& textNode = backTextEntity.emplace<Nz::NodeComponent>();
			textNode.SetParent(entity);
			textNode.SetPosition({ textSprite->GetAABB().width * 0.5f, 1.5f, 0.f });
			textNode.SetRotation(Nz::EulerAnglesf(0.f, Nz::TurnAnglef(0.5f), 0.f));

			backTextEntity.emplace<Nz::GraphicsComponent>(textSprite, playerRenderMask);
		}
		entity.get_or_emplace<EntityOwnerComponent>().Register(backTextEntity);

		if (entityData.controllingPlayerId == m_ownPlayerIndex)
		{
			m_playerControlledEntity = entity;
			OnControlledEntityChanged(entity);
		}
		else
			entity.emplace<NetworkInterpolationComponent>(m_lastTickIndex);

		if (playerInfo)
			playerInfo->textSprite = std::move(textSprite);
	}
}
