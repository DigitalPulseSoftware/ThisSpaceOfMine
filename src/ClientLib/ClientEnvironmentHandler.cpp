// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientEnvironmentHandler.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/PlayerAnimationController.hpp>
#include <ClientLib/RenderConstants.hpp>
#include <ClientLib/Components/AnimationComponent.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <ClientLib/Components/EnvironmentComponent.hpp>
#include <ClientLib/Components/NetworkInterpolationComponent.hpp>
#include <ClientLib/Components/PhysicsInterpolationComponent.hpp>
#include <ClientLib/Components/TransformCopyComponent.hpp>
#include <ClientLib/Components/VisualEntityComponent.hpp>
#include <ClientLib/Entities/ClientChunkClassLibrary.hpp>
#include <ClientLib/Entities/ClientEntityClassLibrary.hpp>
#include <ClientLib/Scripting/ClientAssetScriptingLibrary.hpp>
#include <ClientLib/Scripting/ClientEntityScriptingLibrary.hpp>
#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <CommonLib/AtmosphereScattering.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/EntityOwnerComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Core/Components/SkeletonComponent.hpp>
#include <Nazara/Graphics/DirectionalLight.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/TextSprite.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <spdlog/spdlog.h>

#ifdef TSOM_DEV_TOOLS
#include <imgui.h>
#endif

namespace tsom
{
	ClientEnvironmentHandler::ClientEnvironmentHandler(Nz::ApplicationBase& app, ClientSessionHandler& sessionHandler, Nz::EnttWorld& world, entt::handle cameraEntity, ClientBlockLibrary& blockLibrary) :
	m_cameraEntity(cameraEntity),
	m_blockLibrary(blockLibrary),
	m_sessionHandler(sessionHandler),
	m_app(app),
	m_world(world),
	m_scriptingContext(app)
	{
		m_csmSplitFactors[0] = 0.002f;
		m_csmSplitFactors[1] = 0.006f;
		m_csmSplitFactors[2] = 0.02f;

		SetupPlayerModel();
		SetupRootEntities();

		m_scriptingContext.RegisterLibrary<MathScriptingLibrary>();
		m_scriptingContext.RegisterLibrary<ClientAssetScriptingLibrary>(m_app);
		m_scriptingContext.LoadDirectory("scripts/assets");

		m_entityRegistry.RegisterClassLibrary<ClientChunkClassLibrary>(m_app, m_blockLibrary);
		m_entityRegistry.RegisterClassLibrary<ClientEntityClassLibrary>(m_app);

		m_scriptingContext.RegisterLibrary<ClientEntityScriptingLibrary>(m_entityRegistry);
		m_scriptingContext.RegisterLibrary<ClientScriptingLibrary>(m_app, *this, m_sessionHandler);

		LoadScripts();

		m_onChunkCreate.Connect(sessionHandler.OnChunkCreate, this, Nz::Overload<Packets::S_ChunkCreate&>(&ClientEnvironmentHandler::HandlePacket));
		m_onChunkDestroy.Connect(sessionHandler.OnChunkDestroy, this, Nz::Overload<Packets::S_ChunkDestroy&>(&ClientEnvironmentHandler::HandlePacket));
		m_onChunkReset.Connect(sessionHandler.OnChunkReset, this, Nz::Overload<Packets::S_ChunkReset&>(&ClientEnvironmentHandler::HandlePacket));
		m_onChunkUpdate.Connect(sessionHandler.OnChunkUpdate, this, Nz::Overload<Packets::S_ChunkUpdate&>(&ClientEnvironmentHandler::HandlePacket));
		m_onDebugDrawLineList.Connect(sessionHandler.OnDebugDrawLineList, this, Nz::Overload<Packets::S_DebugDrawLineList&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEntitiesCreation.Connect(sessionHandler.OnEntitiesCreation, this, Nz::Overload<Packets::S_EntitiesCreation&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEntitiesDelete.Connect(sessionHandler.OnEntitiesDelete, this, Nz::Overload<Packets::S_EntitiesDelete&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEntitiesStateUpdate.Connect(sessionHandler.OnEntitiesStateUpdate, this, Nz::Overload<Packets::S_EntitiesStateUpdate&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEntityEnvironmentUpdate.Connect(sessionHandler.OnEntityEnvironmentUpdate, this, Nz::Overload<Packets::S_EntityEnvironmentUpdate&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEntityProcedureCall.Connect(sessionHandler.OnEntityProcedureCall, this, Nz::Overload<Packets::S_EntityProcedureCall&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEntityPropertiesUpdate.Connect(sessionHandler.OnEntityPropertiesUpdate, this, Nz::Overload<Packets::S_EntityPropertiesUpdate&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEnvironmentCreate.Connect(sessionHandler.OnEnvironmentCreate, this, Nz::Overload<Packets::S_EnvironmentCreate&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEnvironmentDestroy.Connect(sessionHandler.OnEnvironmentDestroy, this, Nz::Overload<Packets::S_EnvironmentDestroy&>(&ClientEnvironmentHandler::HandlePacket));
		m_onEnvironmentsUpdateOwner.Connect(sessionHandler.OnEnvironmentsUpdateOwner, this, Nz::Overload<Packets::S_EnvironmentsUpdateOwner&>(&ClientEnvironmentHandler::HandlePacket));
		m_onGameData.Connect(sessionHandler.OnGameData, this, Nz::Overload<Packets::S_GameData&>(&ClientEnvironmentHandler::HandlePacket));
		m_onPilotShip.Connect(sessionHandler.OnPilotShip, this, Nz::Overload<Packets::S_PilotShip&>(&ClientEnvironmentHandler::HandlePacket));
		m_onPlanetEnvironmentRotation.Connect(sessionHandler.OnPlanetEnvironmentRotation, this, Nz::Overload<Packets::S_PlanetEnvironmentRotation&>(&ClientEnvironmentHandler::HandlePacket));
	}

	ClientEnvironmentHandler::~ClientEnvironmentHandler()
	{
		for (auto& entityDataOpt : m_entities)
		{
			if (entityDataOpt)
				entityDataOpt->entity.destroy();
		}
	}

	void ClientEnvironmentHandler::Draw(Nz::Time elapsedTime, Nz::DebugDrawer* debugDrawer)
	{
		for (auto it = m_debugDrawLines.begin(); it != m_debugDrawLines.end();)
		{
			DebugDrawLines& debugDrawLines = it.value();
			debugDrawLines.duration -= elapsedTime;
			if (debugDrawLines.duration < Nz::Time::Zero())
			{
				it = m_debugDrawLines.erase(it);
				continue;
			}

			if (debugDrawLines.environmentId >= m_environments.size() || !m_environments[debugDrawLines.environmentId])
			{
				spdlog::warn("debug draw line for unknown environment {}", debugDrawLines.environmentId);
				it = m_debugDrawLines.erase(it);
				continue;
			}

			auto& env = *m_environments[debugDrawLines.environmentId];
			if (env.isRoot)
			{
				// Fast path, environment node has no transformation (root environment)
				debugDrawer->DrawLines(debugDrawLines.vertices, debugDrawLines.color);
			}
			else
			{
				auto& rootNode = env.visualRootEntity.get<Nz::NodeComponent>();

				// Slow path, transform lines
				for (std::size_t i = 0; i < debugDrawLines.vertices.size(); i += 2)
					debugDrawer->DrawLine(rootNode.ToGlobalPosition(debugDrawLines.vertices[i]), rootNode.ToGlobalPosition(debugDrawLines.vertices[i + 1]), debugDrawLines.color);
			}

			++it;
		}
	}

	void ClientEnvironmentHandler::LoadScripts(bool isReloading)
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

	void ClientEnvironmentHandler::Update(Nz::Time elapsedTime, ImGuiRuntime* imguiRuntime)
	{
#ifdef TSOM_DEV_TOOLS
		if (imguiRuntime)
		{
			ImGui::SetNextWindowPos({ 60, 60 }, ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Directional light", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Cascaded shadow maps");

				if (ImGui::DragFloat3("Split factors", m_csmSplitFactors.data(), 0.0001f, 0.f, 1.f, "%.6f"))
				{
					m_directionalLight->UpdateShadowCascadeFixedSplitFactors(m_csmSplitFactors);
				}
			}
			ImGui::End();
		}
#endif

		for (auto&& environmentOpt : m_environments)
		{
			if (!environmentOpt)
				continue;

			auto& environment = *environmentOpt;
			if (environment.isRoot)
			{
				Nz::Quaternionf inverseRotation(-environment.rotation, environment.rotationAxis);

				m_rootTransform.SetRotation(inverseRotation);

				m_skyboxMaterial->SetValueProperty("Rotation", environment.rotation.ToRadians());
				m_skyboxMaterial->SetValueProperty("RotationAxis", environment.rotationAxis);

				auto view = m_world.GetRegistry().view<AtmosphereScattering>();
				for (auto&& [entity, atmosphereScattering] : view.each())
				{
					atmosphereScattering.sunDir = inverseRotation * Nz::Vector3f(0.852868497f, 0.5f, 0.150383770f);
				}
			}
			else
			{
				auto& visualNode = environment.visualRootEntity.get<Nz::NodeComponent>();
				visualNode.SetRotation(Nz::Quaternionf(environment.rotation, environment.rotationAxis));
			}
		}
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_ChunkCreate& chunkCreate)
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

	void ClientEnvironmentHandler::HandlePacket(Packets::S_ChunkDestroy& chunkDestroy)
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

	void ClientEnvironmentHandler::HandlePacket(Packets::S_ChunkReset& chunkReset)
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

		chunk->LockWrite();
		chunk->Reset([&](BlockIndex* blocks)
		{
			for (BlockIndex blockContent : chunkReset.content)
				*blocks++ = blockContent;
		});
		chunk->UnlockWrite();
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_ChunkUpdate& chunkUpdate)
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

	void ClientEnvironmentHandler::HandlePacket(Packets::S_DebugDrawLineList& debugDrawLineList)
	{
		auto& debugDrawLines = m_debugDrawLines[debugDrawLineList.uniqueHash];
		debugDrawLines.color = debugDrawLineList.color;
		debugDrawLines.duration = Nz::Time::Seconds(debugDrawLineList.duration);
		debugDrawLines.environmentId = debugDrawLineList.environmentId;
		debugDrawLines.vertices.clear();

		if (!debugDrawLineList.indices.empty())
		{
			for (Nz::UInt16 index : debugDrawLineList.indices)
				debugDrawLines.vertices.push_back(debugDrawLineList.vertices[index]);
		}
		else
			debugDrawLines.vertices = debugDrawLineList.vertices;
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EntitiesCreation& entitiesCreation)
	{
		for (auto& entityData : entitiesCreation.entities)
			HandleEntityCreation(entityData);
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EntitiesDelete& entitiesDelete)
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
			spdlog::info("Deleted entity {} from environment {}", entityId, environmentIndex);
		}
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EntitiesStateUpdate& stateUpdate)
	{
		for (auto& entityStates : stateUpdate.entities)
		{
			assert(m_entities[entityStates.entityId]);
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

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EntityEnvironmentUpdate& environmentUpdate)
	{
		assert(m_entities[environmentUpdate.entity]);
		EntityData& entityData = *m_entities[environmentUpdate.entity];
		spdlog::info("Entity {} moved to environment #{} to environment #{}", environmentUpdate.entity, entityData.environmentIndex, environmentUpdate.newEnvironmentId);

		assert(m_environments[entityData.environmentIndex]);
		auto& oldEnvironment = *m_environments[entityData.environmentIndex];
		oldEnvironment.entities.Reset(environmentUpdate.entity);

		assert(m_environments[environmentUpdate.newEnvironmentId]);
		auto& newEnvironment = *m_environments[environmentUpdate.newEnvironmentId];
		newEnvironment.entities.UnboundedSet(environmentUpdate.entity);

		auto& entityNode = entityData.entity.get<Nz::NodeComponent>();
		entityNode.SetParent(newEnvironment.rootEntity, true);

		auto& entityEnv = entityData.entity.get<EnvironmentComponent>();
		entityEnv.environmentIndex = environmentUpdate.newEnvironmentId;

		entityData.environmentIndex = environmentUpdate.newEnvironmentId;
		if (NetworkInterpolationComponent* movementInterpolation = entityData.entity.try_get<NetworkInterpolationComponent>())
			movementInterpolation->UpdateRoot(oldEnvironment.rootEntity.get<Nz::NodeComponent>(), newEnvironment.rootEntity.get<Nz::NodeComponent>());

		auto& entityVisualComp = entityData.entity.get<VisualEntityComponent>();
		auto& visualNode = entityVisualComp.visualEntity.get<Nz::NodeComponent>();
		visualNode.SetParent(newEnvironment.visualRootEntity, true);
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EntityProcedureCall& procedureCall)
	{
		assert(m_entities[procedureCall.entity]);
		EntityData& entityData = *m_entities[procedureCall.entity];

		auto& classInstance = entityData.entity.get<ClassInstanceComponent>();
		const auto& clientRpc = classInstance.GetClass()->GetClientRpc(procedureCall.rpcIndex);
		if (clientRpc.onCalled)
			clientRpc.onCalled(entityData.entity);
		else
			spdlog::warn("client rpc {} has been triggered but has no callback", clientRpc.name);
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EntityPropertiesUpdate& propertyUpdate)
	{
		assert(m_entities[propertyUpdate.entity]);
		EntityData& entityData = *m_entities[propertyUpdate.entity];

		auto& classInstance = entityData.entity.get<ClassInstanceComponent>();
		for (auto& propertyData : propertyUpdate.properties)
			classInstance.UpdateProperty(propertyData.index, std::move(propertyData.value));
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EnvironmentCreate& envCreate)
	{
		spdlog::info("Created environment #{} (owned by {})", envCreate.id, envCreate.ownerEntity);
		if (envCreate.id >= m_environments.size())
			m_environments.resize(envCreate.id + 1);

		auto& environment = m_environments[envCreate.id].emplace();

		environment.rootEntity = m_world.CreateEntity();
		environment.rootEntity.emplace<Nz::NodeComponent>();

		environment.visualRootEntity = m_world.CreateEntity();
		environment.visualRootEntity.emplace<Nz::NodeComponent>();

		if (envCreate.ownerEntity != Nz::MaxValue<Packets::Helper::EntityId>())
		{
			assert(m_entities[envCreate.ownerEntity]);
			EntityData& ownerEntity = *m_entities[envCreate.ownerEntity];

			auto& ownerNode = ownerEntity.entity.get<Nz::NodeComponent>();

			environment.rootEntity.get<Nz::NodeComponent>().SetParent(ownerNode);
			environment.visualRootEntity.get<Nz::NodeComponent>().SetParent(ownerNode);
		}
		else
		{
			environment.isRoot = true;
			m_rootEnvironments.push_back(envCreate.id);
		}

		for (auto& entityData : envCreate.entities)
			HandleEntityCreation(entityData);
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EnvironmentDestroy& envDestroy)
	{
		spdlog::info("Destroyed environment #{}", envDestroy.id);
		for (std::size_t entityIndex : m_environments[envDestroy.id]->entities.IterBits())
		{
			assert(m_entities[entityIndex]);
			EntityData& entityData = *m_entities[entityIndex];
			entityData.entity.destroy();

			m_entities[entityIndex].reset();
		}

		if (auto it = std::find(m_rootEnvironments.begin(), m_rootEnvironments.end(), envDestroy.id); it != m_rootEnvironments.end())
			m_rootEnvironments.erase(it);

		m_environments[envDestroy.id].reset();
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_EnvironmentsUpdateOwner& envOwnerUpdate)
	{
		for (const auto& ownerUpdate : envOwnerUpdate.ownerUpdates)
		{
			spdlog::info("Environment #{} changed owned to {}", ownerUpdate.environment, ownerUpdate.newOwner);

			if (auto it = std::find(m_rootEnvironments.begin(), m_rootEnvironments.end(), ownerUpdate.environment); it != m_rootEnvironments.end())
				m_rootEnvironments.erase(it);

			Nz::Node* ownerNode = nullptr;
			if (ownerUpdate.newOwner != Nz::MaxValue<Packets::Helper::EntityId>())
			{
				NazaraAssert(m_entities[ownerUpdate.newOwner]);
				EntityData& ownerEntity = *m_entities[ownerUpdate.newOwner];
				ownerNode = &ownerEntity.entity.get<Nz::NodeComponent>();
			}
			else
				m_rootEnvironments.push_back(ownerUpdate.environment);

			NazaraAssert(ownerUpdate.environment < m_environments.size() && m_environments[ownerUpdate.environment]);
			auto& envData = *m_environments[ownerUpdate.environment];
			envData.isRoot = (ownerNode == nullptr);
			envData.rootEntity.get<Nz::NodeComponent>().SetParent(ownerNode);
			envData.visualRootEntity.get<Nz::NodeComponent>().SetParent(ownerNode);

			if (envData.isRoot)
			{
				auto& visualNode = envData.visualRootEntity.get<Nz::NodeComponent>();
				visualNode.SetRotation(Nz::Quaternionf::Identity());
			}
		}
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_GameData& gameData)
	{
		m_lastTickIndex = gameData.tickIndex;
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_PlanetEnvironmentRotation& planetEnvironmentRotation)
	{
		if (planetEnvironmentRotation.id >= m_environments.size() || !m_environments[planetEnvironmentRotation.id])
		{
			spdlog::warn("received rotation for unknown environment #{}", planetEnvironmentRotation.id);
			return;
		}

		auto& environment = *m_environments[planetEnvironmentRotation.id];
		environment.rotation = planetEnvironmentRotation.rotation;
		environment.rotationAxis = planetEnvironmentRotation.rotationAxis;
	}

	void ClientEnvironmentHandler::HandlePacket(Packets::S_PilotShip& pilotShip)
	{
		assert(m_entities[pilotShip.shipEntity]);
		entt::handle& entity = m_entities[pilotShip.shipEntity]->entity;
		entt::handle& exteriorEntity = m_entities[pilotShip.shipExteriorEntity]->entity;

		OnPilotShip(entity, exteriorEntity, pilotShip.referenceRotation);
	}

	void ClientEnvironmentHandler::HandleEntityCreation(Packets::Helper::EntityData& entityData)
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

		if (entityData.entityFlags.Test(Packets::Helper::EntityFlags::CancelEnvironmentRotation))
			entityNode.SetParent(m_rootTransform);
		else
			entityNode.SetParent(environment.rootEntity);

		auto& entityEnv = entity.emplace<EnvironmentComponent>();
		entityEnv.environmentIndex = entityData.environmentId;

		auto& entityNetId = entity.emplace<ClientEntityNetworkIndex>();
		entityNetId.networkIndex = entityData.entityId;

		// Create visual entity
		entt::handle visualEntity = m_world.CreateEntity();

		auto& visualNode = visualEntity.emplace<Nz::NodeComponent>(entityData.initialStates.position, entityData.initialStates.rotation);

		if (entityData.entityFlags.Test(Packets::Helper::EntityFlags::CancelEnvironmentRotation))
			visualNode.SetParent(m_rootTransform);
		else
			visualNode.SetParent(environment.visualRootEntity);

		// Bind visual entity to logical entity
		auto& entityVisualComp = entity.emplace<VisualEntityComponent>();
		entityVisualComp.visualEntity = visualEntity;

		auto& entityOwnerComp = entity.emplace<EntityOwnerComponent>();
		entityOwnerComp.Register(visualEntity);

		std::string entityClassName = m_sessionHandler.GetSession()->GetStringStore().GetString(entityData.entityClass);
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
			SetupEntity(entity, *entityData.playerControlled);

		// TEMP
		if (PlanetComponent* planetComponent = entity.try_get<PlanetComponent>())
			environment.gravityController = planetComponent->planet.get();
		else if (ShipComponent* shipComponent = entity.try_get<ShipComponent>())
			environment.gravityController = shipComponent->ship.get();

		spdlog::info("Created entity {} in environment {} ({})", entityData.entityId, entityData.environmentId, entityClassName);

		// Since we make use of parenting for environments, we need to make replication happen in global space
		if (Nz::RigidBody3DComponent* rigidBody = entity.try_get<Nz::RigidBody3DComponent>())
		{
			if (rigidBody->GetReplicationMode() != Nz::PhysicsReplication3D::None)
			{
				auto& referencedInterp = visualEntity.emplace<ReferencedPhysicsInterpolationComponent>();
				referencedInterp.referenceEntity = entity;
			}

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

	void ClientEnvironmentHandler::SetupEntity(entt::handle entity, Packets::Helper::PlayerControlledData& entityData)
	{
		Nz::RigidBody3D::DynamicSettings physSettings(m_playerCollider, 0.f);
		physSettings.objectLayer = Constants::ObjectLayerPlayer;

		entity.emplace<Nz::RigidBody3DComponent>(physSettings, Nz::PhysicsReplication3D::None);

		auto& visualEntityComp = entity.get<VisualEntityComponent>();

		entt::handle& visualEntity = visualEntityComp.visualEntity;

		Nz::UInt32 playerRenderMask = (entityData.controllingPlayerId == m_sessionHandler.GetLocalPlayerIndex()) ? tsom::Constants::RenderMaskLocalPlayer : tsom::Constants::RenderMaskOtherPlayer;

		auto& gfx = visualEntity.emplace<Nz::GraphicsComponent>();
		gfx.AttachRenderable(m_playerModel, playerRenderMask);

		// Skeleton & animations
		std::shared_ptr<Nz::Skeleton> skeleton = std::make_shared<Nz::Skeleton>(m_playerAnimAssets->referenceSkeleton);

		auto& skeletonComponent = visualEntity.emplace<Nz::SkeletonComponent>(skeleton);

		visualEntity.emplace<AnimationComponent>(skeleton, std::make_shared<PlayerAnimationController>(visualEntity, m_playerAnimAssets));

		// Floating name
		std::shared_ptr<Nz::TextSprite> textSprite = std::make_shared<Nz::TextSprite>(Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic, Nz::MaterialInstancePreset::ReverseZ | Nz::MaterialInstancePreset::AlphaBlended));

		ClientSessionHandler::PlayerInfo* playerInfo = m_sessionHandler.FetchPlayerInfo(entityData.controllingPlayerId);
		if (playerInfo)
			textSprite->Update(Nz::SimpleTextDrawer::Draw(playerInfo->nickname, 48, Nz::TextStyle_Regular, (playerInfo->isAuthenticated) ? Nz::Color::White() : Nz::Color::Gray()), 0.01f);
		else
			textSprite->Update(Nz::SimpleTextDrawer::Draw("<disconnected>", 48, Nz::TextStyle_Regular, Nz::Color::Gray()), 0.01f);

		entt::handle frontTextEntity = m_world.CreateEntity();
		{
			auto& textNode = frontTextEntity.emplace<Nz::NodeComponent>();
			textNode.SetParent(visualEntity);
			textNode.SetPosition({ -textSprite->GetAABB().width * 0.5f, 1.5f, 0.f });

			frontTextEntity.emplace<Nz::GraphicsComponent>(textSprite, playerRenderMask);
		}
		visualEntity.get_or_emplace<EntityOwnerComponent>().Register(frontTextEntity);

		entt::handle backTextEntity = m_world.CreateEntity();
		{
			auto& textNode = backTextEntity.emplace<Nz::NodeComponent>();
			textNode.SetParent(visualEntity);
			textNode.SetPosition({ textSprite->GetAABB().width * 0.5f, 1.5f, 0.f });
			textNode.SetRotation(Nz::EulerAnglesf(0.f, Nz::TurnAnglef(0.5f), 0.f));

			backTextEntity.emplace<Nz::GraphicsComponent>(textSprite, playerRenderMask);
		}
		visualEntity.get_or_emplace<EntityOwnerComponent>().Register(backTextEntity);

		if (entityData.controllingPlayerId == m_sessionHandler.GetLocalPlayerIndex())
		{
			m_playerControlledEntity = entity;
			OnControlledEntityChanged(entity);
		}
		else
		{
			entity.emplace<NetworkInterpolationComponent>(m_lastTickIndex);
			visualEntity.emplace<TransformCopyComponent>().referenceEntity = entity;
		}

		if (playerInfo)
			playerInfo->textSprite = std::move(textSprite);
	}
}
