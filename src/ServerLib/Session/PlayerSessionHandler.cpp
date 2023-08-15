// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Session/PlayerSessionHandler.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <CommonLib/VoxelBlock.hpp>

namespace tsom
{
	constexpr SessionHandler::SendAttributeTable s_packetAttributes = SessionHandler::BuildAttributeTable({
		{ PacketIndex<Packets::ChatMessage>,         { 0, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::ChunkCreate>,         { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::ChunkDestroy>,        { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::ChunkUpdate>,         { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesCreation>,    { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesDelete>,      { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::EntitiesStateUpdate>, { 1, 0 } },
		{ PacketIndex<Packets::GameData>,            { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::PlayerJoin>,          { 1, Nz::ENetPacketFlag_Reliable } },
		{ PacketIndex<Packets::PlayerLeave>,         { 1, Nz::ENetPacketFlag_Reliable } },
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
		Chunk* chunk = m_player->GetVisibilityHandler().GetChunkByIndex(mineBlock.chunkId);
		if (!chunk)
			return; //< ignore

		Nz::Vector3ui voxelLoc(mineBlock.voxelLoc.x, mineBlock.voxelLoc.y, mineBlock.voxelLoc.z);

		m_player->GetServerInstance().UpdatePlanetBlock(chunk->GetIndices(), voxelLoc, VoxelBlock::Empty);
	}

	void PlayerSessionHandler::HandlePacket(Packets::PlaceBlock&& placeBlock)
	{
		Chunk* chunk = m_player->GetVisibilityHandler().GetChunkByIndex(placeBlock.chunkId);
		if (!chunk)
			return; //< ignore

		Nz::Vector3ui voxelLoc(placeBlock.voxelLoc.x, placeBlock.voxelLoc.y, placeBlock.voxelLoc.z);

		m_player->GetServerInstance().UpdatePlanetBlock(chunk->GetIndices(), voxelLoc, static_cast<VoxelBlock>(placeBlock.newContent));
	}

	void PlayerSessionHandler::HandlePacket(Packets::SendChatMessage&& playerChat)
	{
		m_player->GetServerInstance().BroadcastChatMessage(std::move(playerChat.message), m_player->GetPlayerIndex());
	}

	void PlayerSessionHandler::HandlePacket(Packets::UpdatePlayerInputs&& playerInputs)
	{
		m_player->HandleInputs(playerInputs.inputs);
	}
}
