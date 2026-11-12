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
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>

namespace tsom
{
	void BlockTool::OnTrigger(TriggerType triggerType)
	{
		auto raycastHit = m_gameInterface.RaycastQuery();
		if (!raycastHit || !raycastHit->hitChunk)
			return;

		auto& chunkNetworkMap = raycastHit->hitEntity.get<ChunkNetworkMapComponent>();
		auto& chunkNode = raycastHit->hitChunkEntity.get<Nz::NodeComponent>();

		const Chunk& hitChunk = *raycastHit->hitChunk;
		const ChunkContainer& chunkContainer = raycastHit->hitChunk->GetContainer();

		Nz::Vector3f localPos = chunkNode.ToLocalPosition(raycastHit->hitPosition);
		Nz::Vector3f localNormal = chunkNode.ToLocalDirection(raycastHit->hitNormal);

		auto hitCoordinates = raycastHit->hitChunk->ComputeHitCoordinates(localPos, localNormal, *raycastHit->hitCollider, raycastHit->subShapeID);
		if (!hitCoordinates)
			return;

		if (triggerType == TriggerType::Primary)
		{
			// Mine
			Packets::C_MineBlock mineBlock;
			mineBlock.chunkId = Nz::Retrieve(chunkNetworkMap.chunkNetworkIndices, &hitChunk);
			mineBlock.voxelLoc.x = hitCoordinates->blockIndices.x;
			mineBlock.voxelLoc.y = hitCoordinates->blockIndices.y;
			mineBlock.voxelLoc.z = hitCoordinates->blockIndices.z;

			m_gameInterface.GetNetworkSession()->SendPacket(mineBlock);
		}
		else if (triggerType == TriggerType::Secondary)
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
		else if (triggerType == ToolBase::TriggerType::Tertiary)
		{
			// Pick block
			BlockIndex pickedBlockIndex = raycastHit->hitChunk->GetBlockContent(hitCoordinates->blockIndices);
			m_gameInterface.GetBlockSelectionBar()->SelectPickedBlock(pickedBlockIndex);
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

	void BlockTool::Update(Nz::Time /*elapsedTime*/, const Physics::RaycastHit* previewRaycast)
	{
		if (!previewRaycast || !previewRaycast->hitChunk)
			return;

		auto& chunkNode = previewRaycast->hitChunkEntity.get<Nz::NodeComponent>();

		const ChunkContainer& chunkContainer = previewRaycast->hitChunk->GetContainer();

		Nz::Vector3f localPos = chunkNode.ToLocalPosition(previewRaycast->hitPosition);
		Nz::Vector3f localNormal = chunkNode.ToLocalDirection(previewRaycast->hitNormal);

		auto hitCoordinates = previewRaycast->hitChunk->ComputeHitCoordinates(localPos, localNormal, *previewRaycast->hitCollider, previewRaycast->subShapeID);
		if (!hitCoordinates)
			return;

		auto cornerPos = previewRaycast->hitChunk->ComputeBlockCorners(hitCoordinates->blockIndices);
		auto& corners = s_faceCorners[hitCoordinates->direction];

		Nz::Vector3f offset = previewRaycast->hitNormal * 0.03f;

		Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[0]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[1]]) + offset, Nz::Color::Green());
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[1]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[2]]) + offset, Nz::Color::Green());
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[2]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[3]]) + offset, Nz::Color::Green());
		debugDrawer->DrawLine(chunkNode.ToGlobalPosition(cornerPos[corners[3]]) + offset, chunkNode.ToGlobalPosition(cornerPos[corners[0]]) + offset, Nz::Color::Green());
	}
}
