// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Planet.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <nlohmann/json.hpp>
#include <fmt/color.h>
#include <charconv>
#include <numeric>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::ChatMessage>,             { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkCreate>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkDestroy>,            { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkReset>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkUpdate>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::DebugDrawLineList>,       { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EntitiesCreation>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EntitiesDelete>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EntitiesStateUpdate>,     { .channel = 1, .flags = Nz::ENetPacketFlag_Unreliable } },
		{ PacketIndex<Packets::EntityEnvironmentUpdate>, { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EnvironmentCreate>,       { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EnvironmentDestroy>,      { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EnvironmentUpdate>,       { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::GameData>,                { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::PlayerJoin>,              { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::PlayerLeave>,             { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::PlayerNameUpdate>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::UpdateRootEnvironment>,   { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
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

	void PlayerSessionHandler::HandlePacket(Packets::MineBlock&& mineBlock)
	{
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

	void PlayerSessionHandler::HandlePacket(Packets::PlaceBlock&& placeBlock)
	{
		Chunk* chunk;
		entt::handle entityOwner;
		if (!m_player->GetVisibilityHandler().GetChunkByNetworkId(placeBlock.chunkId, &entityOwner, &chunk))
			return; //< ignore

		assert(chunk);
		assert(entityOwner);

		ServerEnvironment* environment = entityOwner.registry()->ctx().get<ServerEnvironment*>();

		Nz::Vector3ui voxelLoc(placeBlock.voxelLoc.x, placeBlock.voxelLoc.y, placeBlock.voxelLoc.z);
		if (!CheckCanPlaceBlock(environment, chunk, voxelLoc))
			return;

		chunk->LockWrite();
		chunk->UpdateBlock(voxelLoc, static_cast<BlockIndex>(placeBlock.newContent));
		chunk->UnlockWrite();
	}

	void PlayerSessionHandler::HandlePacket(Packets::SendChatMessage&& playerChat)
	{
		std::string_view message = static_cast<std::string_view>(playerChat.message);
		if (message == "/respawn")
		{
			const auto& spawnpoint = m_player->GetServerInstance().GetDefaultSpawnpoint();
			m_player->UpdateRootEnvironment(spawnpoint.env);
			m_player->Respawn(spawnpoint.env, spawnpoint.position, spawnpoint.rotation);
			return;
		}
		else if (message == "/fly")
		{
			m_player->GetCharacterController()->EnableFlying(!m_player->GetCharacterController()->IsFlying());

			Packets::ChatMessage chatMessage;
			chatMessage.message = (m_player->GetCharacterController()->IsFlying()) ? "fly enabled" : "fly disabled";

			GetSession()->SendPacket(std::move(chatMessage));
			return;
		}
		else if (message == "/spawnship" || message.starts_with("/spawnship "))
		{
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

			entt::handle playerEntity = m_player->GetControlledEntity();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();

			ServerEnvironment* currentEnvironment = m_player->GetControlledEntityEnvironment();
			if (currentEnvironment != m_player->GetRootEnvironment())
				return;

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
					return; //< player disconnected

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
							fmt::print(fg(fmt::color::red), "failed to parse ship data json: {}\n", e.what());
							player->SendChatMessage("failed to load ship (an internal error occurred)");
							return;
						}

						if (auto result = shipEnv->Load(dataDoc); !result)
						{
							fmt::print(fg(fmt::color::red), "failed to parse ship data json: {}\n", result.GetError());
							player->SendChatMessage("failed to load ship (an internal error occurred)");
							return;
						}
					}
					else
						shipEnv->GenerateShip(true);

					ServerEnvironment* currentEnvironment = player->GetControlledEntityEnvironment();
					if (currentEnvironment != player->GetRootEnvironment())
						return;

					EnvironmentTransform planetToShip(spawnPos, spawnRot);
					shipEnv->LinkOutsideEnvironment(currentEnvironment, planetToShip);

					player->SetOwnedShip(std::move(shipEnv));
				}
				else
				{
					fmt::print(fg(fmt::color::red), "failed to parse ship data json: (code {}) {}\n", resultCode, body);
					player->SendChatMessage("failed to load ship (an internal error occurred)");
				}
			});
			return;
		}
		else if (message == "/regenchunk" && m_player->HasPermission(PlayerPermission::Admin))
		{
			entt::handle playerEntity = m_player->GetControlledEntity();
			if (!playerEntity)
				return;

			ServerInstance& serverInstance = m_player->GetServerInstance();
			ServerEnvironment* currentEnvironment = m_player->GetControlledEntityEnvironment();

			// Bad temporary code
			ServerPlanetEnvironment* planetEnvironment = dynamic_cast<ServerPlanetEnvironment*>(currentEnvironment);
			if (!planetEnvironment)
				return;

			Planet& planet = planetEnvironment->GetPlanet();

			Nz::Vector3f playerPos = playerEntity.get<Nz::NodeComponent>().GetGlobalPosition();
			ChunkIndices chunkIndices = planet.GetChunkIndicesByPosition(playerPos);

			const BlockLibrary& blockLibrary = m_player->GetServerInstance().GetBlockLibrary();

			if (Chunk* chunk = planet.GetChunk(chunkIndices))
			{
				planet.GenerateChunk(blockLibrary, *chunk, 42, Nz::Vector3ui(5));
				fmt::print("regenerated chunk {};{};{}\n", chunkIndices.x, chunkIndices.y, chunkIndices.z);
			}
			return;
		}
		else if (message == "/spawnplatform")
		{
// 			entt::handle playerEntity = m_player->GetControlledEntity();
// 			if (playerEntity)
// 			{
// 				PlanetComponent* playerPlanet = playerEntity.try_get<PlanetComponent>();
// 				if (playerPlanet)
// 				{
// 					const BlockLibrary& blockLibrary = m_player->GetServerInstance().GetBlockLibrary();
// 
// 					Nz::Vector3f playerPos = playerEntity.get<Nz::NodeComponent>().GetGlobalPosition();
// 					ChunkIndices chunkIndices = playerPlanet->GetChunkIndicesByPosition(playerPos);
// 					if (Chunk* chunk = playerPlanet->GetChunk(chunkIndices))
// 					{
// 						std::optional<Nz::Vector3ui> coords = chunk->ComputeCoordinates(playerPos);
// 						if (coords)
// 						{
// 							Direction dir = DirectionFromNormal(playerPlanet->ComputeUpDirection(playerPos));
// 							BlockIndices blockIndices = playerPlanet->GetBlockIndices(chunkIndices, *coords);
// 							playerPlanet->GeneratePlatform(blockLibrary, dir, blockIndices);
// 
// 							fmt::print("generated platform at {};{};{}\n", blockIndices.x, blockIndices.y, blockIndices.z);
// 						}
// 					}
// 				}
// 			}
			return;
		}
		else if (message == "/spawnplanet" && m_player->HasPermission(PlayerPermission::Admin))
		{
			entt::handle playerEntity = m_player->GetControlledEntity();
			if (!playerEntity)
				return;

			ServerEnvironment* environment = m_player->GetRootEnvironment();

			entt::handle newPlanetEntity = environment->CreateEntity();
			newPlanetEntity.emplace<Nz::NodeComponent>().CopyTransform(playerEntity.get<Nz::NodeComponent>());
			newPlanetEntity.emplace<NetworkedComponent>();

			auto& planetComponent = newPlanetEntity.emplace<PlanetComponent>();

			ServerInstance& serverInstance = m_player->GetServerInstance();

			auto& taskScheduler = serverInstance.GetApplication().GetComponent<Nz::TaskSchedulerAppComponent>();

			planetComponent.planet = std::make_unique<Planet>(1.f, 16.f, 9.81f);
			planetComponent.planet->GenerateChunks(serverInstance.GetBlockLibrary(), taskScheduler, std::rand(), Nz::Vector3ui(5));

			planetComponent.planetEntities = std::make_unique<ChunkEntities>(serverInstance.GetApplication(), environment->GetWorld(), *planetComponent.planet, serverInstance.GetBlockLibrary());
			planetComponent.planetEntities->SetParentEntity(newPlanetEntity);
			return;
		}

		m_player->GetServerInstance().BroadcastChatMessage(std::move(playerChat.message), m_player->GetPlayerIndex());
	}

	void PlayerSessionHandler::HandlePacket(Packets::UpdatePlayerInputs&& playerInputs)
	{
		m_player->PushInputs(playerInputs.inputs);
	}

	void PlayerSessionHandler::OnDeserializationError(std::size_t packetIndex)
	{
		fmt::print("failed to deserialize unexpected packet {1} from peer {0}\n", GetSession()->GetPeerId(), PacketNames[packetIndex]);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	void PlayerSessionHandler::OnUnexpectedPacket(std::size_t packetIndex)
	{
		fmt::print("received unexpected packet {1} from peer {0}\n", GetSession()->GetPeerId(), PacketNames[packetIndex]);
		GetSession()->Disconnect(DisconnectionType::Kick);
	}

	void PlayerSessionHandler::OnUnknownOpcode(Nz::UInt8 opcode)
	{
		fmt::print("received unknown packet (opcode: {1}) from peer {0}\n", GetSession()->GetPeerId(), +opcode);
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
		Nz::BoxCollider3D boxCollider(Nz::Vector3f(chunk->GetBlockSize() * 0.75f)); // Test a smaller block to allow a bit of overlap

		auto corners = chunk->ComputeVoxelCorners(blockIndices);
		Nz::Vector3f blockCenter = std::accumulate(corners.begin(), corners.end(), Nz::Vector3f::Zero()) / corners.size();
		Nz::Vector3f offset = chunk->GetContainer().GetChunkOffset(chunk->GetIndices());

		struct IgnoreTrigger : Nz::PhysObjectLayerFilter3D
		{
			bool ShouldCollide(Nz::PhysObjectLayer3D objectLayer) const override
			{
				return objectLayer != Constants::ObjectLayerStaticTrigger;
			}
		};

		IgnoreTrigger physObjectLayerFilter;

		auto& physicsSystem = environment->GetWorld().GetSystem<Nz::Physics3DSystem>();
		bool doesCollide = physicsSystem.CollisionQuery(boxCollider, Nz::Matrix4f::Translate(offset + blockCenter), [](const Nz::Physics3DSystem::ShapeCollisionInfo& hitInfo) -> std::optional<float>
		{
			return hitInfo.penetrationDepth;
		}, nullptr, &physObjectLayerFilter);

		if (doesCollide)
			return false;

		return true;
	}
}
