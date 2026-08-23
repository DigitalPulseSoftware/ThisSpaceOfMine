// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/RemoveEntityTool.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>

namespace tsom
{
	void RemoveEntityTool::OnTrigger(bool primary)
	{
		if (!m_targetEntity)
			return;

		const ClientEntityNetworkIndex* entityNetwork = m_targetEntity.try_get<const ClientEntityNetworkIndex>();
		if (entityNetwork)
		{
			Packets::C_RemoveEntity removeEntity;
			removeEntity.entityId = entityNetwork->networkIndex;

			m_gameInterface.GetNetworkSession()->SendPacket(removeEntity);
		}
	}

	void RemoveEntityTool::Update(Nz::Time /*elapsedTime*/, const GameInterface::RaycastResult* previewRaycast)
	{
		m_targetEntity = {};
		if (!previewRaycast || !previewRaycast->hitEntity)
			return;

		if (!previewRaycast->hitEntity.all_of<ClientEntityNetworkIndex>())
			return;

		if (Nz::GraphicsComponent* gfxComponent = previewRaycast->hitEntity.try_get<Nz::GraphicsComponent>())
		{
			Nz::NodeComponent& visualNode = previewRaycast->hitEntity.get<Nz::NodeComponent>();

			Nz::Boxf aabb = gfxComponent->GetAABB();
			aabb.ScaleAroundCenter(1.05f);

			auto aabbCorners = aabb.GetCorners();
			for (Nz::Vector3f& corner : aabbCorners)
				corner = visualNode.ToGlobalPosition(corner);

			Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();
			debugDrawer->DrawBoxCorners(aabbCorners, Nz::Color::Red());

			m_targetEntity = previewRaycast->hitEntity;
		}
	}
}
