// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/ConnectTool.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <ClientLib/Components/VisualEntityComponent.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>

namespace tsom
{
	void ConnectTool::OnTrigger(bool primary)
	{
		if (!m_hoveredEntity)
			return;

		if (m_selectedSourceEntity)
		{
			if (m_selectedSourceEntity == m_hoveredEntity)
			{
				m_selectedSourceEntity = {};
				return;
			}

			// Connect entity
			Packets::C_ConnectEntities connectEntities;
			connectEntities.sourceEntityId = m_selectedSourceEntity.get<const ClientEntityNetworkIndex>().networkIndex;
			connectEntities.targetEntityId = m_hoveredEntity.get<const ClientEntityNetworkIndex>().networkIndex;
			connectEntities.sourceEntitySlot = 0;
			connectEntities.targetEntitySlot = 0;

			m_gameInterface.GetNetworkSession()->SendPacket(connectEntities);

			m_selectedSourceEntity = {};
		}
		else
			m_selectedSourceEntity = m_hoveredEntity;
	}

	void ConnectTool::Update(Nz::Time /*elapsedTime*/, const GameInterface::RaycastResult* previewRaycast)
	{
		if (m_selectedSourceEntity)
		{
			DrawEntityAABB(m_selectedSourceEntity, Nz::Color::Magenta());

			if (previewRaycast)
			{
				Nz::NodeComponent& entityNode = m_selectedSourceEntity.get<Nz::NodeComponent>();

				Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();
				debugDrawer->DrawLine(entityNode.GetGlobalPosition(), previewRaycast->hitPos, Nz::Color::Blue());
			}
		}

		m_hoveredEntity = {};
		if (!previewRaycast || !previewRaycast->hitEntity)
			return;

		if (!previewRaycast->hitEntity.all_of<ClientEntityNetworkIndex>())
			return;

		m_hoveredEntity = previewRaycast->hitEntity;
		DrawEntityAABB(m_hoveredEntity, Nz::Color::Green());
	}

	void ConnectTool::DrawEntityAABB(entt::handle entity, Nz::Color color)
	{
		entt::handle visualEntity = entity;
		if (VisualEntityComponent* visualComponent = visualEntity.try_get<VisualEntityComponent>())
			visualEntity = visualComponent->visualEntity;

		if (Nz::GraphicsComponent* gfxComponent = visualEntity.try_get<Nz::GraphicsComponent>())
		{
			Nz::NodeComponent& entityNode = visualEntity.get<Nz::NodeComponent>();

			Nz::Boxf aabb = gfxComponent->GetAABB();
			aabb.ScaleAroundCenter(1.05f);

			auto aabbCorners = aabb.GetCorners();
			for (Nz::Vector3f& corner : aabbCorners)
				corner = entityNode.ToGlobalPosition(corner);

			Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();
			debugDrawer->DrawBoxCorners(aabbCorners, color);
		}
	}
}
