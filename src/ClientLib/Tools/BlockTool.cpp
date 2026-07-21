// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/BlockTool.hpp>
#include <ClientLib/BlockSelectionBar.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Game/States/GameState.hpp>
#include <Game/States/StateData.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>

namespace tsom
{
	void BlockTool::OnTrigger(bool primary)
	{
		auto raycastHit = m_gameInterface.RaycastQuery();
		if (!raycastHit)
			return;

		auto* chunkComponent = raycastHit->hitEntity.try_get<ChunkComponent>();
		if (!chunkComponent)
			return;

		auto& chunkNetworkMap = chunkComponent->parentEntity.get<ChunkNetworkMapComponent>();
		auto& chunkRigidBody = raycastHit->hitEntity.get<Nz::RigidBody3DComponent>();
		auto& chunkNode = raycastHit->hitEntity.get<Nz::NodeComponent>();

		const Chunk& hitChunk = *chunkComponent->chunk;
		const ChunkContainer& chunkContainer = hitChunk.GetContainer();

		Nz::Vector3f localPos = chunkNode.ToLocalPosition(raycastHit->hitPos);
		Nz::Vector3f localNormal = chunkNode.ToLocalDirection(raycastHit->hitNormal);

		auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), raycastHit->subShapeID);
		if (!hitCoordinates)
			return;

		if (primary)
		{
			// Mine
			Packets::C_MineBlock mineBlock;
			mineBlock.chunkId = Nz::Retrieve(chunkNetworkMap.chunkNetworkIndices, &hitChunk);
			mineBlock.voxelLoc.x = hitCoordinates->blockIndices.x;
			mineBlock.voxelLoc.y = hitCoordinates->blockIndices.y;
			mineBlock.voxelLoc.z = hitCoordinates->blockIndices.z;

			m_gameInterface.GetNetworkSession()->SendPacket(mineBlock);
		}
		else
		{
			BlockIndices blockIndices = chunkContainer.GetBlockIndices(hitChunk.GetIndices(), hitCoordinates->blockIndices);

			const DirectionAxis& dirAxis = s_dirAxis[hitCoordinates->direction];

			blockIndices[dirAxis.upAxis] += dirAxis.upDir;

			Nz::Vector3ui innerCoordinates;
			ChunkIndices chunkIndices = chunkContainer.GetChunkIndicesByBlockIndices(blockIndices, &innerCoordinates);
			const Chunk* chunk = chunkContainer.GetChunk(chunkIndices);
			if (!chunk)
				return;

			Packets::C_PlaceBlock placeBlock;
			placeBlock.chunkId = Nz::Retrieve(chunkNetworkMap.chunkNetworkIndices, chunk);
			placeBlock.voxelLoc.x = innerCoordinates.x;
			placeBlock.voxelLoc.y = innerCoordinates.y;
			placeBlock.voxelLoc.z = innerCoordinates.z;
			placeBlock.newContent = Nz::SafeCast<Nz::UInt8>(m_gameInterface.GetBlockSelectionBar()->GetSelectedBlock());

			m_gameInterface.GetNetworkSession()->SendPacket(placeBlock);
		}
	}

	void BlockTool::OnWheel(float delta)
	{
		BlockSelectionBar* blockSelectionBar = m_gameInterface.GetBlockSelectionBar();
		if (blockSelectionBar->IsVisible())
		{
			if (delta < 0.f)
				blockSelectionBar->SelectNext();
			else
				blockSelectionBar->SelectPrevious();
		}
	}

	void BlockTool::Update(Nz::Time /*elapsedTime*/, const GameInterface::RaycastResult* previewRaycast)
	{
		if (!previewRaycast)
			return;

		auto* chunkComponent = previewRaycast->hitEntity.try_get<ChunkComponent>();
		if (!chunkComponent)
			return;

		auto& chunkRigidBody = previewRaycast->hitEntity.get<Nz::RigidBody3DComponent>();
		auto& chunkNode = previewRaycast->hitEntity.get<Nz::NodeComponent>();

		const Chunk& hitChunk = *chunkComponent->chunk;
		const ChunkContainer& chunkContainer = hitChunk.GetContainer();

		Nz::Vector3f localPos = chunkNode.ToLocalPosition(previewRaycast->hitPos);
		Nz::Vector3f localNormal = chunkNode.ToLocalDirection(previewRaycast->hitNormal);

		auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), previewRaycast->subShapeID);
		if (!hitCoordinates)
			return;

		auto cornerPos = hitChunk.ComputeBlockCorners(hitCoordinates->blockIndices);
		auto& corners = s_faceCorners[hitCoordinates->direction];

		Nz::Vector3f offset = previewRaycast->hitNormal * 0.03f;

		Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[0]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[1]]) + offset, Nz::Color::Green());
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[1]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[2]]) + offset, Nz::Color::Green());
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[2]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[3]]) + offset, Nz::Color::Green());
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[3]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[0]]) + offset, Nz::Color::Green());
	}
}
