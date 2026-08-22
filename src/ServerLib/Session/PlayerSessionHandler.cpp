// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/ChunkLock.hpp>
#include <CommonLib/Direction.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/TickComponent.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/DatabaseComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerInteractibleComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/PhysConstraint3D.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <charconv>
#include <numeric>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::S_ChatMessage>,              { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkCreate>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkDestroy>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkReset>,               { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkUpdate>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ConsoleOutput>,            { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_DebugDrawLineList>,        { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntitiesCreation>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntitiesDelete>,           { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntitiesStateUpdate>,      { .channel = 1, .flags = Nz::ENetPacketFlag_Unreliable } },
		{ PacketIndex<Packets::S_EntityEnvironmentUpdate>,  { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntityDistributionUpdate>, { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntityProcedureCall>,      { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntityPropertiesUpdate>,   { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EnvironmentCreate>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EnvironmentDestroy>,       { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EnvironmentsUpdateOwner>,  { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_GameData>,                 { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PilotShip>,                { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PilotShipFinish>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PlayerJoin>,               { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PlayerLeave>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PlayerNameUpdate>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
	});

	PlayerSessionHandler::PlayerSessionHandler(NetworkSession* session, ServerPlayer* player) :
	SessionHandler(session),
	m_player(player)
	{
		SetupHandlerTable(this);
		SetupAttributeTable(s_packetAttributes);
	}

	PlayerSessionHandler::~PlayerSessionHandler()
	{
		m_player->Destroy();
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_ConnectEntities&& connectEntities)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		entt::handle sourceEntity;
		if (!m_player->GetVisibilityHandler().GetEntityByNetworkId(connectEntities.sourceEntityId, &sourceEntity))
		{
			spdlog::error("Player {} tried to connect unkown entity {}", m_player->GetNickname(), connectEntities.sourceEntityId);
			return;
		}

		entt::handle targetEntity;
		if (!m_player->GetVisibilityHandler().GetEntityByNetworkId(connectEntities.targetEntityId, &targetEntity))
		{
			spdlog::error("Player {} tried to connect unkown entity {}", m_player->GetNickname(), connectEntities.targetEntityId);
			return;
		}

		DistributionComponent* sourceDistribution = sourceEntity.try_get<DistributionComponent>();
		DistributionComponent* targetDistribution = targetEntity.try_get<DistributionComponent>();
		if (!sourceDistribution)
		{
			spdlog::error("Player {} tried to connect entity {} which has no distribution component", m_player->GetNickname(), connectEntities.sourceEntityId);
			return;
		}

		if (!targetDistribution)
		{
			spdlog::error("Player {} tried to connect entity {} which has no distribution component", m_player->GetNickname(), connectEntities.targetEntityId);
			return;
		}

		if (connectEntities.sourceEntityPort >= sourceDistribution->GetOutputCount())
		{
			spdlog::error("Player {} tried to connect entity {} on output port {} but it only has {} ports", m_player->GetNickname(), connectEntities.sourceEntityId, connectEntities.sourceEntityPort, sourceDistribution->GetOutputCount());
			return;
		}

		if (connectEntities.targetEntityPort >= targetDistribution->GetInputCount())
		{
			spdlog::error("Player {} tried to connect entity {} on input port {} but it only has {} ports", m_player->GetNickname(), connectEntities.targetEntityId, connectEntities.targetEntityPort, targetDistribution->GetOutputCount());
			return;
		}

		DistributionComponent::Connect(sourceEntity, *sourceDistribution, targetEntity, *targetDistribution, connectEntities.sourceEntityPort, connectEntities.targetEntityPort);
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_EntityProcedureCall&& entityProcedureCall)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		entt::handle entity;
		if (!m_player->GetVisibilityHandler().GetEntityByNetworkId(entityProcedureCall.entity, &entity))
		{
			spdlog::warn("received RPC call on non-existent entity {}", entityProcedureCall.entity);
			return;
		}

		ClassInstanceComponent* classInstanceComponent = entity.try_get<ClassInstanceComponent>();
		if (!classInstanceComponent)
		{
			spdlog::warn("blocked RPC request on entity {} from player {}", entityProcedureCall.entity, m_player->GetNickname());
			return;
		}

		if (entityProcedureCall.rpcIndex >= classInstanceComponent->GetClass()->GetServerRpcCount())
		{
			spdlog::warn("blocked RPC request on entity {} from player {} (invalid rpc #{})", entityProcedureCall.entity, m_player->GetNickname(), *entityProcedureCall.rpcIndex);
			return;
		}

		// TODO
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_ExitShipControl&& exitShipControl)
	{
		m_player->ExitPiloting();
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_GrabEntity&& /*grab*/)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return;

		if (m_player->HasGrabbedEntity())
		{
			m_player->Ungrab();
			return;
		}

		const auto& characterController = m_player->GetCharacterController();
		Nz::Quaternionf cameraRot = characterController->GetCameraRotation();

		Nz::Vector3f hitPos;
		entt::handle hitEntity;
		auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
		{
			if (hitInfo.hitEntity.try_get<Nz::RigidBody3DComponent>())
			{
				hitPos = hitInfo.hitPosition;
				hitEntity = hitInfo.hitEntity;
			}
		};

		struct ObjectFilter : Nz::PhysObjectLayerFilter3D
		{
			bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
			{
				return layer == Constants::ObjectLayerDynamic;
			}
		};
		ObjectFilter objectFilter;

		auto& playerNode = playerEntity.get<Nz::NodeComponent>();

		Nz::Vector3f cameraPos = characterController->GetEyePosition();

		ServerEnvironment* environment = m_player->GetRootEnvironment();

		auto& physSystem = environment->GetWorld().GetSystem<Nz::Physics3DSystem>();
		if (physSystem.RaycastQueryFirst(cameraPos, cameraPos + cameraRot * Nz::Vector3f::Forward() * 10.f, callback, nullptr, &objectFilter))
		{
			Nz::Vector3f localPos = cameraRot.GetConjugate() * (hitPos - cameraPos);
			m_player->GrabEntity(hitEntity, localPos);
		}
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_Interact&& interact)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		entt::handle entity;
		if (!m_player->GetVisibilityHandler().GetEntityByNetworkId(interact.entityId, &entity))
			return;

		ServerInteractibleComponent* interactibleEntity = entity.try_get<ServerInteractibleComponent>();
		if (!interactibleEntity || !interactibleEntity->isEnabled)
		{
			spdlog::warn("blocked interact request on non-interactible entity {} from player {}", entt::to_integral(entity.entity()), m_player->GetNickname());
			return;
		}

		if (!interactibleEntity->onInteraction)
		{
			spdlog::warn("entity {} has no interaction callback", entt::to_integral(entity.entity()));
			return;
		}

		interactibleEntity->onInteraction(entity, m_player);
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_MineBlock&& mineBlock)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		Chunk* chunk;
		if (!m_player->GetVisibilityHandler().GetChunkByNetworkId(mineBlock.chunkId, nullptr, &chunk))
			return; //< ignore

		assert(chunk);

		Nz::Vector3ui voxelLoc(mineBlock.voxelLoc.x, mineBlock.voxelLoc.y, mineBlock.voxelLoc.z);
		if (!CheckCanMineBlock(chunk, voxelLoc))
			return;

		ChunkWriteLock lock(chunk);

		chunk->SetFlags(ChunkFlag::PlayerModified | ChunkFlag::SaveToDatabase);
		chunk->UpdateBlock(voxelLoc, EmptyBlockIndex);
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_PlaceBlock&& placeBlock)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		Chunk* chunk;
		entt::handle entityOwner;
		if (!m_player->GetVisibilityHandler().GetChunkByNetworkId(placeBlock.chunkId, &entityOwner, &chunk))
			return; //< ignore

		assert(chunk);
		assert(entityOwner);

		ServerEnvironment* environment = ServerEnvironment::GetEnvironment(entityOwner);

		Nz::Vector3ui voxelLoc(placeBlock.voxelLoc.x, placeBlock.voxelLoc.y, placeBlock.voxelLoc.z);
		if (!CheckCanPlaceBlock(environment, chunk, voxelLoc))
			return;

		ChunkWriteLock lock(chunk);

		chunk->SetFlags(ChunkFlag::PlayerModified | ChunkFlag::SaveToDatabase);
		chunk->UpdateBlock(voxelLoc, static_cast<BlockIndex>(placeBlock.newContent));
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_PlaceEntity&& placeEntity)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		Chunk* chunk;
		entt::handle entityOwner;
		if (!m_player->GetVisibilityHandler().GetChunkByNetworkId(placeEntity.chunkId, &entityOwner, &chunk))
			return; //< ignore

		const std::string& className = GetSession()->GetStringStore().GetString(placeEntity.entityClass);

		auto& serverInstance = m_player->GetServerInstance();
		std::shared_ptr<const EntityClass> entityClass = serverInstance.GetEntityRegistry().FindClass(className);
		if (!entityClass)
			return;

		ServerEnvironment* chunkEnvironment = ServerEnvironment::GetEnvironment(entityOwner);

		const Nz::ParameterList& metadata = entityClass->GetMetadata();
		auto colliderXResult = metadata.GetDoubleParameter("spawnable_collider.x");
		auto colliderYResult = metadata.GetDoubleParameter("spawnable_collider.y");
		auto colliderZResult = metadata.GetDoubleParameter("spawnable_collider.z");
		bool keepUpright = metadata.GetBooleanParameter("spawnable_keepupright").GetValueOr(false);

		Nz::Vector3f collider;
		collider.x = static_cast<float>(colliderXResult.GetValueOr(1.0));
		collider.y = static_cast<float>(colliderYResult.GetValueOr(1.0));
		collider.z = static_cast<float>(colliderZResult.GetValueOr(1.0));

		Nz::EulerAnglesf rotationOffset;
		rotationOffset.pitch = static_cast<float>(metadata.GetDoubleParameter("spawnable_rotation.x").GetValueOr(0.0));
		rotationOffset.yaw = static_cast<float>(metadata.GetDoubleParameter("spawnable_rotation.y").GetValueOr(0.0));
		rotationOffset.roll = static_cast<float>(metadata.GetDoubleParameter("spawnable_rotation.z").GetValueOr(0.0));

		Nz::Vector3ui blockIndices(placeEntity.voxelLoc.x, placeEntity.voxelLoc.y, placeEntity.voxelLoc.z);
		if (!CheckCanPlaceEntity(chunkEnvironment, chunk, blockIndices, placeEntity.topFace, collider, placeEntity.entityRotation))
			return;

		auto& chunkOwnerNode = entityOwner.get<Nz::NodeComponent>();

		Nz::Vector3f chunkOffset = chunk->GetContainer().GetChunkOffset(chunk->GetIndices());

		auto cornerPos = chunk->ComputeBlockCorners(blockIndices);
		auto& corners = s_faceCorners[placeEntity.topFace];
		std::array<Nz::Vector3f, 4> cornerGlobalPos;
		for (std::size_t i = 0; i < 4; ++i)
			cornerGlobalPos[i] = chunkOwnerNode.ToGlobalPosition(chunkOffset + cornerPos[corners[i]]);

		Nz::Vector3f faceCenter = std::accumulate(cornerGlobalPos.begin(), cornerGlobalPos.end(), Nz::Vector3f::Zero()) / corners.size();

		Nz::Vector3f normal = s_dirNormals[placeEntity.topFace];

		Nz::Quaternionf surfaceRotation = Nz::Quaternionf::Identity();
		Nz::Vector3f entityPos = faceCenter;

		if (keepUpright)
			entityPos += normal * collider * 0.5f;
		else
		{
			entityPos += normal * collider.y * 0.5f;
			surfaceRotation = Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), normal);
		}

		Nz::Quaternionf entityRot = surfaceRotation * Nz::Quaternionf(Nz::DegreeAnglef(placeEntity.entityRotation * 45.f), Nz::Vector3f::Up()) * Nz::Quaternionf(rotationOffset);

		entt::handle entity = chunkEnvironment->CreateEntity();
		entity.emplace<Nz::NodeComponent>(entityPos, entityRot);
		entity.emplace<NetworkedComponent>();
		entity.emplace<ClassInstanceComponent>(entityClass);
		entity.emplace<DatabaseComponent>();

		entityClass->InitAndActivateEntity(entity);
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_RemoveEntity&& removeEntity)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		entt::handle entity;
		if (!m_player->GetVisibilityHandler().GetEntityByNetworkId(removeEntity.entityId, &entity))
			return;

		if (entity)
			entity.destroy();
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_SendChatMessage&& playerChat)
	{
		std::string_view message = static_cast<std::string_view>(playerChat.message);
		if (message == "/respawn")
		{
			const auto& spawnpoint = m_player->GetServerInstance().GetDefaultSpawnpoint();
			m_player->UpdateRootEnvironment(spawnpoint.env);
			m_player->Respawn(spawnpoint.env, spawnpoint.position, spawnpoint.rotation);
			return;
		}
		else if (message.starts_with("/tpplanet "))
		{
			constexpr std::size_t commandLength = sizeof("/tpplanet ") - 1;

			Nz::UInt32 planetId;
			const char* last = message.data() + message.size();
			std::from_chars_result res = std::from_chars(&message[commandLength], last, planetId);
			if (res.ptr != last || res.ec != std::errc{})
			{
				m_player->SendChatMessage("failed to parse planet id");
				return;
			}

			auto& serverInstance = m_player->GetServerInstance();
			ServerEnvironment* env = serverInstance.FindEnvironmentFromDatabaseId(planetId);
			if (!env)
			{
				m_player->SendChatMessage("invalid planet id");
				return;
			}

			Nz::Boxf planetAABB = env->ComputeBoundingBox();

			Nz::Vector3f planetCenter = planetAABB.GetCenter();
			Nz::Vector3f spawnPos = planetCenter + (planetAABB.GetRadius() + Constants::PlayerColliderHeight * 2.f) * Nz::Vector3f::Up();

			auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
			{
				spawnPos = hitInfo.hitPosition + hitInfo.hitNormal * Constants::PlayerColliderHeight;
			};

			struct IgnorePlayer : Nz::PhysObjectLayerFilter3D
			{
				bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
				{
					return layer != Constants::ObjectLayerPlayer;
				}
			};
			IgnorePlayer objectFilter;

			auto& physSystem = env->GetWorld().GetSystem<Nz::Physics3DSystem>();
			physSystem.RaycastQueryFirst(spawnPos, planetCenter, callback, nullptr, &objectFilter);

			m_player->UpdateRootEnvironment(env);
			m_player->Respawn(env, spawnPos, Nz::Quaternionf::Identity());
		}
		else if (message.starts_with("/tpplayer ") && m_player->HasPermission(PlayerPermission::Admin))
		{
			constexpr std::size_t commandLength = sizeof("/tpplayer ") - 1;

			std::string targetName(&message[commandLength], message.size() - commandLength);
			ServerPlayer* targetPlayer = m_player->GetServerInstance().FindPlayerByNickname(targetName);
			if (!targetPlayer)
			{
				m_player->SendChatMessage("player not found");
				return;
			}

			entt::handle targetEntity = targetPlayer->GetControlledEntityReference();
			if (!targetEntity)
			{
				m_player->SendChatMessage("target player has no controlled entity");
				return;
			}

			Nz::Vector3f targetPos = targetEntity.get<Nz::NodeComponent>().GetPosition();
			ServerEnvironment* targetEnvironment = ServerEnvironment::GetEnvironment(targetEntity);
			m_player->Respawn(targetEnvironment, targetPos + Nz::Vector3f::Up() * 2.f, Nz::Quaternionf::Identity());

			return;
		}
		else if (message == "/fly")
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return; //< player is either dead or not spawned yet

			m_player->GetCharacterController()->EnableFlying(!m_player->GetCharacterController()->IsFlying());

			Packets::S_ChatMessage chatMessage;
			chatMessage.message = (m_player->GetCharacterController()->IsFlying()) ? "fly enabled" : "fly disabled";

			GetSession()->SendPacket(std::move(chatMessage));
			return;
		}
		else if (message == "/spawnship" || message.starts_with("/spawnship "))
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return; //< player is either dead or not spawned yet

			constexpr std::size_t commandLength = sizeof("/spawnship ") - 1;

			int slot = 0;
			if (message.starts_with("/spawnship ") && message.size() > commandLength)
			{
				const char* last = message.data() + message.size();
				std::from_chars_result res = std::from_chars(&message[commandLength], last, slot);
				if (res.ptr != last || res.ec != std::errc{})
				{
					m_player->SendChatMessage("failed to parse ship slot");
					return;
				}

				if (slot < 0 || slot >= 3)
				{
					m_player->SendChatMessage("slot must lie in [0;3[");
					return;
				}
			}

			ServerInstance& serverInstance = m_player->GetServerInstance();

			ServerEnvironment* currentEnvironment = m_player->GetControlledEntityEnvironment();
			if (currentEnvironment != m_player->GetRootEnvironment())
			{
				spdlog::warn("player {} tried to spawn ship but their entity is not part of their root environment", m_player->GetNickname());
				return;
			}

			Nz::NodeComponent& playerNode = playerEntity.get<Nz::NodeComponent>();
			Nz::Vector3f spawnPos = playerNode.GetPosition();
			Nz::Quaternionf spawnRot = Nz::Quaternionf::RotationBetween(Nz::Vector3f::Down(), playerNode.GetDown());

			if (!m_player->IsAuthenticated())
			{
				m_player->SendChatMessage("warning: your ship won't be saved as you're not authenticated");

				// spawn ship like before saving
				auto shipEnv = std::make_unique<ServerShipEnvironment>(serverInstance, std::nullopt, slot);
				shipEnv->GenerateShip(true);

				EnvironmentTransform planetToShip(spawnPos, spawnRot);
				shipEnv->LinkOutsideEnvironment(currentEnvironment, planetToShip);

				m_player->SetOwnedShip(std::move(shipEnv));
				return;
			}

			PlayerTokenAppComponent& playerToken = serverInstance.GetApplication().GetComponent<PlayerTokenAppComponent>();

			playerToken.QueueRequest(*m_player->GetUuid(), Nz::WebRequestMethod::Get, fmt::format("/v1/player_ship/{}", slot), {}, [&serverInstance, slot, spawnPos, spawnRot, player = m_player->CreateHandle(), playerEntity](Nz::UInt32 resultCode, const std::string& body)
			{
				if (!player || !playerEntity)
				{
					spdlog::warn("player tried to spawn ship disconnected before the database answer", player->GetNickname());
					return;
				}

				if (resultCode == 200 || resultCode == 404)
				{
					const BlockLibrary& blockLibrary = serverInstance.GetBlockLibrary();

					Nz::NodeComponent& playerNode = playerEntity.get<Nz::NodeComponent>();

					// don't allow to save ship when logged in as dev
					std::optional<Nz::Uuid> playerUuid;
					if (!player->HasPermission(PlayerPermission::Dev))
						playerUuid = player->GetUuid();
					else
						player->SendChatMessage("warning: your ship won't be saved (using dev mode)");

					auto shipEnv = std::make_unique<ServerShipEnvironment>(serverInstance, playerUuid, slot);

					if (resultCode == 200)
					{
						nlohmann::json dataDoc;
						try
						{
							dataDoc = nlohmann::json::parse(body);
							dataDoc = nlohmann::json::parse(std::string(dataDoc["ship_data"]));
						}
						catch (const std::exception& e)
						{
							spdlog::error("failed to parse ship data json: {}", e.what());
							player->SendChatMessage("failed to load ship (an internal error occurred)");
							return;
						}

						if (auto result = shipEnv->Load(dataDoc); !result)
						{
							spdlog::error("failed to parse ship data json: {}", result.GetError());
							player->SendChatMessage("failed to load ship (an internal error occurred)");
							return;
						}

						if (shipEnv->IsEmpty())
							shipEnv->GenerateShip(true);
					}
					else
						shipEnv->GenerateShip(true);

					ServerEnvironment* currentEnvironment = player->GetControlledEntityEnvironment();
					if (currentEnvironment != player->GetRootEnvironment())
					{
						spdlog::error("player {} tried to spawn ship but their entity is not part of their root environment (after database request)", player->GetNickname());
						return;
					}

					EnvironmentTransform planetToShip(spawnPos, spawnRot);
					shipEnv->LinkOutsideEnvironment(currentEnvironment, planetToShip);

					player->SetOwnedShip(std::move(shipEnv));
				}
				else
				{
					spdlog::error("failed to parse ship data json: (code {}) {}", resultCode, body);
					player->SendChatMessage("failed to load ship (an internal error occurred)");
				}
			});
			return;
		}
		else if (message == "/regenchunk" && m_player->HasPermission(PlayerPermission::Admin))
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();
			ServerEnvironment* currentEnvironment = ServerEnvironment::GetEnvironment(playerEntity);
			if (currentEnvironment->GetType() != ServerEnvironmentType::Planet)
				return;

			ServerPlanetEnvironment* planetEnvironment = static_cast<ServerPlanetEnvironment*>(currentEnvironment);
			if (!planetEnvironment)
				return;

			Planet& planet = planetEnvironment->GetPlanet();

			Nz::Vector3f playerPos = playerEntity.get<Nz::NodeComponent>().GetGlobalPosition();
			ChunkIndices chunkIndices = planet.GetChunkIndicesByPosition(playerPos);

			const BlockLibrary& blockLibrary = m_player->GetServerInstance().GetBlockLibrary();

			if (Chunk* chunk = planet.GetChunk(chunkIndices))
			{
				std::optional<Nz::UInt32> planetId = planetEnvironment->GetDatabaseId();
				serverInstance.GetThreadServerDatabase().GetAllPlanets([&](Database::Planet&& planetData)
				{
					if (planetData.id == planetId)
					{
						//planet.GenerateChunk(blockLibrary, *chunk, planetData.seed, planetData.chunkCount, planetData.generatorName.data());
						spdlog::info("regenerated chunk {};{};{}", chunkIndices.x, chunkIndices.y, chunkIndices.z);
						return false;
					}
					return true;
				});
			}
			return;
		}
		else if (message == "/spawncomputer")
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();

			ServerEnvironment* currentEnvironment = ServerEnvironment::GetEnvironment(playerEntity);
			if (currentEnvironment->GetType() != ServerEnvironmentType::Ship)
			{
				m_player->SendChatMessage("computers can only be spawned in ships");
				return;
			}

			std::shared_ptr<const EntityClass> computerClass = serverInstance.GetEntityRegistry().FindClass("computer");
			if (!computerClass)
				return;

			// Temporary: Destroy previous computer(s) if existing
			entt::registry& environmentRegistry = currentEnvironment->GetWorld().GetRegistry();
			auto classInstanceView = environmentRegistry.view<ClassInstanceComponent>();
			for (entt::entity entity : classInstanceView)
			{
				auto& classInstance = classInstanceView.get<ClassInstanceComponent>(entity);
				if (classInstance.GetClass() == computerClass)
				{
					environmentRegistry.destroy(entity);
				}
			}

			const auto& characterController = m_player->GetCharacterController();
			Nz::Quaternionf cameraRot = characterController->GetCameraRotation();

			Nz::Vector3f hitPos, hitNormal;
			entt::handle hitEntity;
			std::uint32_t hitSubshapeID;
			auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
			{
				hitPos = hitInfo.hitPosition;
				hitNormal = hitInfo.hitNormal;
				hitEntity = hitInfo.hitEntity;
				hitSubshapeID = hitInfo.subShapeID;
			};

			struct IgnorePlayer : Nz::PhysObjectLayerFilter3D
			{
				bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
				{
					return layer != Constants::ObjectLayerPlayer;
				}
			};
			IgnorePlayer objectFilter;

			auto& playerNode = playerEntity.get<Nz::NodeComponent>();

			Nz::Vector3f cameraPos = characterController->GetEyePosition();

			auto& physSystem = currentEnvironment->GetWorld().GetSystem<Nz::Physics3DSystem>();
			if (physSystem.RaycastQueryFirst(cameraPos, cameraPos + cameraRot * Nz::Vector3f::Forward() * 10.f, callback, nullptr, &objectFilter))
			{
				if (auto* chunkComponent = hitEntity.try_get<ChunkComponent>())
				{
					auto& chunkRigidBody = hitEntity.get<Nz::RigidBody3DComponent>();
					auto& chunkNode = hitEntity.get<Nz::NodeComponent>();

					const Chunk& hitChunk = *chunkComponent->chunk;
					const ChunkContainer& chunkContainer = hitChunk.GetContainer();

					Nz::Vector3f localPos = chunkNode.ToLocalPosition(hitPos);
					Nz::Vector3f localNormal = chunkNode.ToLocalDirection(hitNormal);

					auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), hitSubshapeID);
					if (!hitCoordinates)
						return;

					BlockIndices blockIndices = chunkContainer.GetBlockIndices(hitChunk.GetIndices(), hitCoordinates->blockIndices);

					const DirectionAxis& dirAxis = s_dirAxis[hitCoordinates->direction];
					blockIndices[dirAxis.upAxis] += dirAxis.upDir;

					Nz::Vector3ui innerCoordinates;
					ChunkIndices chunkIndices = chunkContainer.GetChunkIndicesByBlockIndices(blockIndices, &innerCoordinates);
					const Chunk* chunk = chunkContainer.GetChunk(chunkIndices);
					if (!chunk)
						return;

					auto corners = chunk->ComputeBlockCorners(innerCoordinates);
					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();
					Nz::Vector3f offset = chunk->GetContainer().GetChunkOffset(chunk->GetIndices());

					Direction dir = DirectionFromNormal(playerNode.GetForward());

					entt::handle entity = currentEnvironment->CreateEntity();
					entity.emplace<Nz::NodeComponent>(blockCenter + offset, Nz::Quaternionf::RotationBetween(Nz::Vector3f::Forward(), s_dirNormals[dir]));
					entity.emplace<NetworkedComponent>();
					entity.emplace<DatabaseComponent>();

					entity.emplace<ClassInstanceComponent>(computerClass);
					computerClass->InitAndActivateEntity(entity);
				}
			}

			return;
		}
		else if (message == "/spawnsensor")
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();

			ServerEnvironment* currentEnvironment = ServerEnvironment::GetEnvironment(playerEntity);

			std::shared_ptr<const EntityClass> atmosphereSensor = serverInstance.GetEntityRegistry().FindClass("atmosphere_sensor");
			if (!atmosphereSensor)
				return;

			const auto& characterController = m_player->GetCharacterController();
			Nz::Quaternionf cameraRot = characterController->GetCameraRotation();

			Nz::Vector3f hitPos, hitNormal;
			entt::handle hitEntity;
			std::uint32_t hitSubshapeID;
			auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
			{
				hitPos = hitInfo.hitPosition;
				hitNormal = hitInfo.hitNormal;
				hitEntity = hitInfo.hitEntity;
				hitSubshapeID = hitInfo.subShapeID;
			};

			struct IgnorePlayer : Nz::PhysObjectLayerFilter3D
			{
				bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
				{
					return layer != Constants::ObjectLayerPlayer;
				}
			};
			IgnorePlayer objectFilter;

			auto& playerNode = playerEntity.get<Nz::NodeComponent>();

			Nz::Vector3f cameraPos = characterController->GetEyePosition();

			auto& physSystem = currentEnvironment->GetWorld().GetSystem<Nz::Physics3DSystem>();
			if (physSystem.RaycastQueryFirst(cameraPos, cameraPos + cameraRot * Nz::Vector3f::Forward() * 10.f, callback, nullptr, &objectFilter))
			{
				if (auto* chunkComponent = hitEntity.try_get<ChunkComponent>())
				{
					auto& chunkRigidBody = hitEntity.get<Nz::RigidBody3DComponent>();
					auto& chunkNode = hitEntity.get<Nz::NodeComponent>();

					const Chunk& hitChunk = *chunkComponent->chunk;
					const ChunkContainer& chunkContainer = hitChunk.GetContainer();

					Nz::Vector3f localPos = chunkNode.ToLocalPosition(hitPos);
					Nz::Vector3f localNormal = chunkNode.ToLocalDirection(hitNormal);

					auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), hitSubshapeID);
					if (!hitCoordinates)
						return;

					BlockIndices blockIndices = chunkContainer.GetBlockIndices(hitChunk.GetIndices(), hitCoordinates->blockIndices);

					const DirectionAxis& dirAxis = s_dirAxis[hitCoordinates->direction];
					blockIndices[dirAxis.upAxis] += dirAxis.upDir;

					Nz::Vector3ui innerCoordinates;
					ChunkIndices chunkIndices = chunkContainer.GetChunkIndicesByBlockIndices(blockIndices, &innerCoordinates);
					const Chunk* chunk = chunkContainer.GetChunk(chunkIndices);
					if (!chunk)
						return;

					auto corners = chunk->ComputeBlockCorners(innerCoordinates);
					Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();
					Nz::Vector3f offset = chunk->GetContainer().GetChunkOffset(chunk->GetIndices());

					Direction dir = DirectionFromNormal(playerNode.GetForward());

					entt::handle entity = currentEnvironment->CreateEntity();
					entity.emplace<Nz::NodeComponent>(blockCenter + offset, Nz::Quaternionf::RotationBetween(Nz::Vector3f::Forward(), s_dirNormals[dir]));
					entity.emplace<NetworkedComponent>();

					entity.emplace<ClassInstanceComponent>(atmosphereSensor);
					atmosphereSensor->InitAndActivateEntity(entity);
				}
			}

			return;
		}
		else if (message == "/spawntree" && m_player->HasPermission(PlayerPermission::Admin))
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();

			ServerEnvironment* currentEnvironment = ServerEnvironment::GetEnvironment(playerEntity);

			std::shared_ptr<const EntityClass> treeClass = serverInstance.GetEntityRegistry().FindClass("tree");
			if (!treeClass)
				return;

			const auto& characterController = m_player->GetCharacterController();
			Nz::Quaternionf cameraRot = characterController->GetCameraRotation();

			Nz::Vector3f hitPos, hitNormal;
			entt::handle hitEntity;
			std::uint32_t hitSubshapeID;
			auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
			{
				hitPos = hitInfo.hitPosition;
				hitNormal = hitInfo.hitNormal;
				hitEntity = hitInfo.hitEntity;
				hitSubshapeID = hitInfo.subShapeID;
			};

			struct IgnorePlayer : Nz::PhysObjectLayerFilter3D
			{
				bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
				{
					return layer != Constants::ObjectLayerPlayer;
				}
			};
			IgnorePlayer objectFilter;

			auto& playerNode = playerEntity.get<Nz::NodeComponent>();

			Nz::Vector3f cameraPos = characterController->GetEyePosition();

			auto& physSystem = currentEnvironment->GetWorld().GetSystem<Nz::Physics3DSystem>();
			if (physSystem.RaycastQueryFirst(cameraPos, cameraPos + cameraRot * Nz::Vector3f::Forward() * 10.f, callback, nullptr, &objectFilter))
			{
				entt::handle entity = currentEnvironment->CreateEntity();
				entity.emplace<Nz::NodeComponent>(hitPos, Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), hitNormal));
				entity.emplace<NetworkedComponent>();
				entity.emplace<DatabaseComponent>();

				auto& treeInstance = entity.emplace<ClassInstanceComponent>(treeClass);
				treeInstance.UpdateProperty<EntityPropertyType::Float>("scale", (float) std::rand() / RAND_MAX + 0.5f);

				treeClass->InitAndActivateEntity(entity);
			}

			return;
		}
		else if (message == "/removetree" && m_player->HasPermission(PlayerPermission::Admin))
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();

			ServerEnvironment* currentEnvironment = ServerEnvironment::GetEnvironment(playerEntity);

			std::shared_ptr<const EntityClass> treeClass = serverInstance.GetEntityRegistry().FindClass("tree");
			if (!treeClass)
				return;

			const auto& characterController = m_player->GetCharacterController();
			Nz::Quaternionf cameraRot = characterController->GetCameraRotation();

			Nz::Vector3f hitPos;
			auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
			{
				hitPos = hitInfo.hitPosition;
			};

			struct IgnorePlayer : Nz::PhysObjectLayerFilter3D
			{
				bool ShouldCollide(Nz::PhysObjectLayer3D layer) const override
				{
					return layer != Constants::ObjectLayerPlayer;
				}
			};
			IgnorePlayer objectFilter;

			auto& playerNode = playerEntity.get<Nz::NodeComponent>();

			Nz::Vector3f cameraPos = characterController->GetEyePosition();

			auto& physSystem = currentEnvironment->GetWorld().GetSystem<Nz::Physics3DSystem>();
			if (physSystem.RaycastQueryFirst(cameraPos, cameraPos + cameraRot * Nz::Vector3f::Forward() * 10.f, callback, nullptr, &objectFilter))
			{
				entt::registry& environmentRegistry = currentEnvironment->GetWorld().GetRegistry();
				auto classInstanceView = environmentRegistry.view<Nz::NodeComponent, ClassInstanceComponent>();
				for (auto&& [entity, entityNode, classInstance] : classInstanceView.each())
				{
					if (classInstance.GetClass() == treeClass && entityNode.GetPosition().SquaredDistance(hitPos) < Nz::IntegralPow(0.5f, 2))
					{
						environmentRegistry.destroy(entity);
						m_player->SendChatMessage("Tree removed");
						return;
					}
				}
			}

			m_player->SendChatMessage("No tree found");
			return;
		}
		else if (message == "/spawnplatform" && m_player->HasPermission(PlayerPermission::Admin))
		{
/*
			entt::handle playerEntity = m_player->GetControlledEntity();
			if (playerEntity)
			{
				PlanetComponent* playerPlanet = playerEntity.try_get<PlanetComponent>();
				if (playerPlanet)
				{
					const BlockLibrary& blockLibrary = m_player->GetServerInstance().GetBlockLibrary();

					Nz::Vector3f playerPos = playerEntity.get<Nz::NodeComponent>().GetGlobalPosition();
					ChunkIndices chunkIndices = playerPlanet->GetChunkIndicesByPosition(playerPos);
					if (Chunk* chunk = playerPlanet->GetChunk(chunkIndices))
					{
						std::optional<Nz::Vector3ui> coords = chunk->ComputeCoordinates(playerPos);
						if (coords)
						{
							Direction dir = DirectionFromNormal(playerPlanet->ComputeUpDirection(playerPos));
							BlockIndices blockIndices = playerPlanet->GetBlockIndices(chunkIndices, *coords);
							playerPlanet->GeneratePlatform(blockLibrary, dir, blockIndices);

							spdlog::info("generated platform at {};{};{}", blockIndices.x, blockIndices.y, blockIndices.z);
						}
					}
				}
			}
*/
			return;
		}
		else if (message == "/crashserver" && m_player->HasPermission(PlayerPermission::Admin))
		{
			int* ptr = (int*) 0x01;
			*ptr = 42;
		}
		else if (message == "/crashserver 1" && m_player->HasPermission(PlayerPermission::Admin))
		{
			throw std::runtime_error("test exception");
		}
		else if (message == "/links")
		{
			m_player->SendChatMessage("--- Game ressources ---");
			m_player->SendChatMessage("Join our community on Discord : http://discord.gg/2WUU6Sp");
			m_player->SendChatMessage("Check out the wiki : https://tsom.fandom.com/fr");
			m_player->SendChatMessage("To suggest a new feature or an idea : https://tsom-idea.digitalpulse.software");
			m_player->SendChatMessage("To report an issue : https://github.com/DigitalPulseSoftware/ThisSpaceOfMine/issues");
			return;
		}
		else if (message == "/help")
		{
			m_player->SendChatMessage("--- Available commands ---");
			m_player->SendChatMessage("/help - Show this help message");
			m_player->SendChatMessage("/links - Show game resources");
			m_player->SendChatMessage("/respawn - Respawn at the default spawnpoint");
			m_player->SendChatMessage("/tpplanet <planet_id> - Teleport to the specified planet");
			m_player->SendChatMessage("/fly - Toggle flying mode");
			m_player->SendChatMessage("/spawnship [slot] - Spawn your ship from the specified slot (default: 0). Slot must be in [0;3[");
			m_player->SendChatMessage("/spawncomputer - Spawn a computer where you're looking at");
			m_player->SendChatMessage("/spawnsensor - Spawn an atmosphere sensor where you're looking at");

			if (m_player->HasPermission(PlayerPermission::Admin))
			{
				m_player->SendChatMessage("/tpplayer <player_name> - Teleport to the specified player");
				m_player->SendChatMessage("/regenchunk - Regenerate the chunk you're currently in");
				m_player->SendChatMessage("/spawntree - Spawn a tree where you're looking at");
				m_player->SendChatMessage("/removetree - Remove a tree where you're looking at");
				m_player->SendChatMessage("/spawnplatform - Spawn a platform at your position (deprecated)");
				m_player->SendChatMessage("/crashserver - Crash the server (for testing purposes)");
				m_player->SendChatMessage("/crashserver 1 - Throw an exception on the server (for testing purposes)");
			}
			return;
		}

		m_player->GetServerInstance().BroadcastChatMessage(std::move(playerChat.message), m_player->GetPlayerIndex());
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_SendConsoleCommand&& consoleCommand)
	{
		if (!m_player->HasPermission(PlayerPermission::Admin))
			return;

		m_player->ExecuteConsoleCommand(consoleCommand.command);
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_ToggleFlashlight&& playerInputs)
	{
		entt::handle playerEntity = m_player->GetControlledEntityReference();
		if (!playerEntity)
			return; //< player is either dead or not spawned yet

		using BoolProperty = EntityPropertySingleValue<EntityPropertyType::Bool>;

		//if (std::get<BoolProperty>(entityInstance.GetProperty(flashlightProperty)).value)

		ClassInstanceComponent& playerInstance = playerEntity.get<ClassInstanceComponent>();
		Nz::UInt32 flashlightProperty = playerInstance.FindPropertyIndex("flashlight");
		NazaraAssert(flashlightProperty != playerInstance.InvalidIndex);

		bool isFlashlightEnabled = std::get<BoolProperty>(playerInstance.GetProperty(flashlightProperty)).value;
		playerInstance.UpdateProperty(flashlightProperty, BoolProperty(!isFlashlightEnabled));
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_UpdatePlayerInputs&& playerInputs)
	{
		m_player->PushInputs(playerInputs.inputs);
	}

	void PlayerSessionHandler::OnDeserializationError(std::size_t packetIndex)
	{
		spdlog::warn("failed to deserialize unexpected packet {1} from peer {0}", GetSession()->GetPeerId(), PacketNames[packetIndex]);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	void PlayerSessionHandler::OnUnexpectedPacket(std::size_t packetIndex)
	{
		spdlog::warn("received unexpected packet {1} from peer {0}", GetSession()->GetPeerId(), PacketNames[packetIndex]);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	void PlayerSessionHandler::OnUnknownOpcode(Nz::UInt8 opcode)
	{
		spdlog::warn("received unknown packet (opcode: {1}) from peer {0}", GetSession()->GetPeerId(), +opcode);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	bool PlayerSessionHandler::CheckCanMineBlock(const Chunk* chunk, const Nz::Vector3ui& blockIndices) const
	{
		Nz::Vector3ui chunkSize = chunk->GetSize();
		if (blockIndices.x >= chunkSize.x || blockIndices.y >= chunkSize.y || blockIndices.z >= chunkSize.z)
			return false;

		// Check that target block is not empty
		if (!chunk->HasContent() || chunk->GetBlockContent(blockIndices) == EmptyBlockIndex)
			return false;

		return true;
	}

	bool PlayerSessionHandler::CheckCanPlaceBlock(ServerEnvironment* environment, const Chunk* chunk, const Nz::Vector3ui& blockIndices) const
	{
		Nz::Vector3ui chunkSize = chunk->GetSize();
		if (blockIndices.x >= chunkSize.x || blockIndices.y >= chunkSize.y || blockIndices.z >= chunkSize.z)
			return false;

		// Check that target block is empty
		if (chunk->HasContent() && chunk->GetBlockContent(blockIndices) != EmptyBlockIndex)
			return false;

		return true; //< FIXME

		// Check that nothing blocks the way
		auto [blockCollider, colliderOffset] = chunk->BuildBlockCollider(blockIndices, 0.75f); // Test a smaller block to allow a bit of overlap

		Nz::Vector3f offset = chunk->GetContainer().GetChunkOffset(chunk->GetIndices());
		offset += colliderOffset;

		struct IgnoreTrigger : Nz::PhysObjectLayerFilter3D
		{
			bool ShouldCollide(Nz::PhysObjectLayer3D objectLayer) const override
			{
				return objectLayer != Constants::ObjectLayerStaticTrigger;
			}
		};

		IgnoreTrigger physObjectLayerFilter;

		auto& physicsSystem = environment->GetWorld().GetSystem<Nz::Physics3DSystem>();
		bool doesCollide = physicsSystem.CollisionQuery(*blockCollider, Nz::Matrix4f::Translate(offset), [](const Nz::Physics3DSystem::ShapeCollisionInfo& hitInfo) -> std::optional<float>
		{
			return hitInfo.penetrationDepth;
		}, nullptr, &physObjectLayerFilter);

		/*Packets::DebugDrawLineList debugDrawLineList;
		debugDrawLineList.color = (doesCollide) ? Nz::Color::Red() : Nz::Color::Green();
		debugDrawLineList.duration = 5.f;
		debugDrawLineList.position = offset;
		debugDrawLineList.rotation = Nz::Quaternionf::Identity();
		blockCollider->BuildDebugMesh(debugDrawLineList.vertices, debugDrawLineList.indices, Nz::Matrix4f::Identity());

		auto& visibility = m_player->GetVisibilityHandler();

		debugDrawLineList.environmentId = visibility.GetEnvironmentId(environment);
		m_player->GetSession()->SendPacket(debugDrawLineList);*/

		if (doesCollide)
			return false;

		return true;
	}

	bool PlayerSessionHandler::CheckCanPlaceEntity(ServerEnvironment* environment, const Chunk* chunk, const Nz::Vector3ui& blockIndices, Direction direction, const Nz::Vector3f& collider, Nz::UInt8 entityRotation) const
	{
		Nz::Vector3ui chunkSize = chunk->GetSize();
		if (blockIndices.x >= chunkSize.x || blockIndices.y >= chunkSize.y || blockIndices.z >= chunkSize.z)
			return false;

		// Check that target block is full
		if (chunk->GetBlockContent(blockIndices) == EmptyBlockIndex)
			return false;

		// TODO
		return true;
	}
}
