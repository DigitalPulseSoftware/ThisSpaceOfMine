// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ChunkEntities.hpp>
#include <CommonLib/Ship.hpp>
#include <CommonLib/Components/PlanetGravityComponent.hpp>
#include <CommonLib/Components/ShipComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Collider3D.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <numeric>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::ChatMessage>,         { .channel = 0, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkCreate>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkDestroy>,        { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkReset>,          { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::ChunkUpdate>,         { .channel = 1, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EntitiesCreation>,    { .channel = 2, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EntitiesDelete>,      { .channel = 2, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::EntitiesStateUpdate>, { .channel = 2, .flags = Nz::ENetPacketFlag_Unreliable } },
		{ PacketIndex<Packets::GameData>,            { .channel = 2, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::PlayerJoin>,          { .channel = 2, .flags = Nz::ENetPacketFlag::Reliable } },
		{ PacketIndex<Packets::PlayerLeave>,         { .channel = 2, .flags = Nz::ENetPacketFlag::Reliable } },
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
		const Chunk* targetChunk = m_player->GetVisibilityHandler().GetChunkByIndex(mineBlock.chunkId);
		if (!targetChunk)
			return; //< ignore

		Nz::Vector3ui voxelLoc(mineBlock.voxelLoc.x, mineBlock.voxelLoc.y, mineBlock.voxelLoc.z);
		if (!CheckCanMineBlock(targetChunk, voxelLoc))
			return;

		if (Chunk* chunk = m_player->GetServerInstance().GetPlanet().GetChunk(targetChunk->GetIndices()))
		{
			chunk->LockWrite();
			chunk->UpdateBlock(voxelLoc, EmptyBlockIndex);
			chunk->UnlockWrite();
		}
	}

	void PlayerSessionHandler::HandlePacket(Packets::PlaceBlock&& placeBlock)
	{
		const Chunk* targetChunk = m_player->GetVisibilityHandler().GetChunkByIndex(placeBlock.chunkId);
		if (!targetChunk)
			return; //< ignore

		Nz::Vector3ui voxelLoc(placeBlock.voxelLoc.x, placeBlock.voxelLoc.y, placeBlock.voxelLoc.z);
		if (!CheckCanPlaceBlock(targetChunk, voxelLoc))
			return;

		if (Chunk* chunk = m_player->GetServerInstance().GetPlanet().GetChunk(targetChunk->GetIndices()))
		{
			chunk->LockWrite();
			chunk->UpdateBlock(voxelLoc, static_cast<BlockIndex>(placeBlock.newContent));
			chunk->UnlockWrite();
		}
	}

	void PlayerSessionHandler::HandlePacket(Packets::SendChatMessage&& playerChat)
	{
		std::string_view message = static_cast<std::string_view>(playerChat.message);
		if (message == "/respawn")
		{
			m_player->Respawn();
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
		else if (message == "/spawnship")
		{
			entt::handle controlledEntity = m_player->GetControlledEntity();
			entt::registry* reg = controlledEntity.registry();

			entt::handle ent = entt::handle(*reg, reg->create());
			auto& node = ent.emplace<Nz::NodeComponent>();
			node.SetPosition(controlledEntity.get<Nz::NodeComponent>().GetPosition());

			ServerInstance& serverInstance = m_player->GetServerInstance();
			Nz::ApplicationBase& app = serverInstance.GetApplication();
			const BlockLibrary& blockLibrary = serverInstance.GetBlockLibrary();

			auto& shipComp = ent.emplace<ShipComponent>();
			shipComp.ship = std::make_unique<Ship>(blockLibrary, Nz::Vector3ui(32), 1.f);

			std::shared_ptr<Nz::Collider3D> chunkCollider = shipComp.ship->GetChunk(0)->BuildCollider(blockLibrary);

			ent.emplace<Nz::RigidBody3DComponent>(Nz::RigidBody3DComponent::DynamicSettings(chunkCollider, 100.f));

			ent.emplace<NetworkedComponent>();
			ent.emplace<PlanetGravityComponent>().planet = &serverInstance.GetPlanet();

			//shipComp.chunkEntities = std::make_unique<ChunkEntities>(app, serverInstance.GetWorld(), *shipComp.ship, blockLibrary);
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

	bool PlayerSessionHandler::CheckCanPlaceBlock(const Chunk* chunk, const Nz::Vector3ui& blockIndices) const
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

		auto& instance = m_player->GetServerInstance();
		auto& physicsSystem = instance.GetWorld().GetSystem<Nz::Physics3DSystem>();
		bool doesCollide = physicsSystem.CollisionQuery(boxCollider, Nz::Matrix4f::Translate(offset + blockCenter), [](const Nz::Physics3DSystem::ShapeCollisionInfo& hitInfo) -> std::optional<float>
		{
			return hitInfo.penetrationDepth;
		});

		if (doesCollide)
			return false;

		return true;
	}
}
