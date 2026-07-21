// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/GrabEntityTool.hpp>
#include <ClientLib/Components/ChunkNetworkMapComponent.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>

namespace tsom
{
	void GrabEntityTool::OnTrigger(bool primary)
	{
		if (!primary)
			return;

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

		const ClientEntityNetworkIndex* entityNetwork = raycastHit->hitEntity.try_get<const ClientEntityNetworkIndex>();
		if (entityNetwork)
		{
			auto& entityNetId = raycastHit->hitEntity.get<ClientEntityNetworkIndex>();

			Packets::C_GrabEntity grabEntity;
			m_gameInterface.GetNetworkSession()->SendPacket(grabEntity);
		}
	}
}
