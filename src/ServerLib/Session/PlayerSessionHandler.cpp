// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
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
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <charconv>
#include <numeric>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::S_ChatMessage>,             { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkCreate>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkDestroy>,            { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkReset>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ChunkUpdate>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_ConsoleOutput>,           { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_DebugDrawLineList>,       { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntitiesCreation>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntitiesDelete>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntitiesStateUpdate>,     { .channel = 1, .flags = Nz::ENetPacketFlag_Unreliable } },
		{ PacketIndex<Packets::S_EntityEnvironmentUpdate>, { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntityProcedureCall>,     { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EntityPropertiesUpdate>,  { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EnvironmentCreate>,       { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EnvironmentDestroy>,      { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_EnvironmentsUpdateOwner>, { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_GameData>,                { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PilotShip>,               { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PilotShipFinish>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PlayerJoin>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PlayerLeave>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::S_PlayerNameUpdate>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
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

	void PlayerSessionHandler::HandlePacket(Packets::C_ExitShipControl&& exitShipControl)
	{
		m_player->ExitPiloting();
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

		chunk->LockWrite();
		chunk->UpdateBlock(voxelLoc, EmptyBlockIndex);
		chunk->UnlockWrite();
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

		chunk->LockWrite();
		chunk->UpdateBlock(voxelLoc, static_cast<BlockIndex>(placeBlock.newContent));
		chunk->UnlockWrite();
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
			Nz::Vector3f spawnPos = planetCenter + planetAABB.GetRadius() * Nz::Vector3f::Up();

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
				auto shipEnv = std::make_unique<ServerShipEnvironment>(serverInstance, m_player->GetUuid(), slot);
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

					auto shipEnv = std::make_unique<ServerShipEnvironment>(serverInstance, player->GetUuid(), slot);

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
				planet.GenerateChunk(blockLibrary, *chunk, 42, Nz::Vector3ui(5));
				spdlog::info("regenerated chunk {};{};{}", chunkIndices.x, chunkIndices.y, chunkIndices.z);
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
		else if (message == "/spawnplanet" && m_player->HasPermission(PlayerPermission::Admin))
		{
			entt::handle playerEntity = m_player->GetControlledEntityReference();
			if (!playerEntity)
				return;

			ServerEnvironment* environment = m_player->GetRootEnvironment();

			entt::handle newPlanetEntity = environment->CreateEntity();
			newPlanetEntity.emplace<Nz::NodeComponent>().CopyTransform(playerEntity.get<Nz::NodeComponent>());
			newPlanetEntity.emplace<NetworkedComponent>();

			ServerInstance& serverInstance = m_player->GetServerInstance();

			auto& taskScheduler = serverInstance.GetApplication().GetComponent<Nz::TaskSchedulerAppComponent>();

			auto& planetComponent = newPlanetEntity.emplace<PlanetComponent>();
			planetComponent.planet = std::make_unique<Planet>(serverInstance.GetApplication(), 1.f, 16.f, 9.81f);
			planetComponent.planet->GenerateChunks(serverInstance.GetBlockLibrary(), taskScheduler, std::rand(), Nz::Vector3ui(5), "alice");

			planetComponent.planetEntities[0] = std::make_unique<ChunkEntities>(serverInstance.GetApplication(), environment->GetWorld(), *planetComponent.planet, serverInstance.GetBlockLibrary(), 0);
			planetComponent.planetEntities[0]->SetParentEntity(newPlanetEntity);
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

		m_player->GetServerInstance().BroadcastChatMessage(std::move(playerChat.message), m_player->GetPlayerIndex());
	}

	void PlayerSessionHandler::HandlePacket(Packets::C_SendConsoleCommand&& consoleCommand)
	{
		if (!m_player->HasPermission(PlayerPermission::Admin))
			return;

		m_player->ExecuteConsoleCommand(consoleCommand.command);
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
		if (chunk->GetBlockContent(blockIndices) == EmptyBlockIndex)
			return false;

		return true;
	}

	bool PlayerSessionHandler::CheckCanPlaceBlock(ServerEnvironment* environment, const Chunk* chunk, const Nz::Vector3ui& blockIndices) const
	{
		Nz::Vector3ui chunkSize = chunk->GetSize();
		if (blockIndices.x >= chunkSize.x || blockIndices.y >= chunkSize.y || blockIndices.z >= chunkSize.z)
			return false;

		// Check that target block is empty
		if (chunk->GetBlockContent(blockIndices) != EmptyBlockIndex)
			return false;

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
}
