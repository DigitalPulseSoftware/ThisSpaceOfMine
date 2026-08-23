// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Tools/ConnectTool.hpp>
#include <ClientLib/Components/ClientEntityNetworkIndex.hpp>
#include <CommonLib/EntityClass.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Graphics/Components/GraphicsComponent.hpp>
#include <Nazara/Renderer/DebugDrawer.hpp>
#include <Nazara/TextRenderer/RichTextBuilder.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>

namespace tsom
{
	void ConnectTool::OnActivate()
	{
		m_labelWidget = m_gameInterface.GetCanvas()->Add<Nz::LabelWidget>();
	}

	void ConnectTool::OnDeactivate()
	{
		m_labelWidget->Destroy();
		m_labelWidget = nullptr;
	}

	void ConnectTool::OnTrigger(TriggerType triggerType)
	{
		if (triggerType == TriggerType::Secondary)
		{
			m_selectedSourceEntity = {};
			UpdateText();
			return;
		}

		if (triggerType != TriggerType::Primary)
			return;

		if (!m_hoveredEntity)
			return;

		if (m_selectedSourceEntity)
		{
			if (m_selectedSourceEntity == m_hoveredEntity)
			{
				m_selectedSourceEntity = {};
				UpdateText();
				return;
			}

			// Connect entity
			Packets::C_ConnectEntities connectEntities;
			connectEntities.sourceEntityId = m_selectedSourceEntity.get<const ClientEntityNetworkIndex>().networkIndex;
			connectEntities.targetEntityId = m_hoveredEntity.get<const ClientEntityNetworkIndex>().networkIndex;
			connectEntities.sourceEntityPort = Nz::SafeCaster(m_sourceEntityPort);
			connectEntities.targetEntityPort = Nz::SafeCaster(m_hoveredEntityPort);

			m_gameInterface.GetNetworkSession()->SendPacket(connectEntities);

			m_selectedSourceEntity = {};
		}
		else
		{
			m_selectedSourceEntity = m_hoveredEntity;
			m_sourceEntityPort = m_hoveredEntityPort;

			m_hoveredEntity = {};
			m_hoveredEntityPort = 0;
		}

		UpdateText();
	}

	void ConnectTool::OnWheel(float delta)
	{
		if (!m_hoveredEntity)
			return;

		DistributionComponent& hoveredDistribution = m_hoveredEntity.get<DistributionComponent>();
		if (delta > 0.f)
		{
			if (m_hoveredEntityPort > 0)
			{
				m_hoveredEntityPort = m_hoveredEntityPort - 1;
				UpdateText();
			}
		}
		else
		{
			std::size_t portCount = (m_selectedSourceEntity) ? hoveredDistribution.GetInputCount() : hoveredDistribution.GetOutputCount();
			if (m_hoveredEntityPort + 1 < portCount)
			{
				m_hoveredEntityPort = m_hoveredEntityPort + 1;
				UpdateText();
			}
		}
	}

	void ConnectTool::Update(Nz::Time /*elapsedTime*/, const GameInterface::RaycastResult* previewRaycast)
	{
		Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();

		if (m_selectedSourceEntity)
		{
			DrawEntityAABB(m_selectedSourceEntity, Nz::Color::Magenta());

			if (previewRaycast)
			{
				Nz::NodeComponent& entityNode = m_selectedSourceEntity.get<Nz::NodeComponent>();

				debugDrawer->DrawLine(entityNode.GetGlobalPosition(), previewRaycast->hitPos, Nz::Color::Blue());
			}
		}

		if (!previewRaycast || !previewRaycast->hitEntity || !previewRaycast->hitEntity.all_of<ClientEntityNetworkIndex, DistributionComponent>())
		{
			m_hoveredEntity = {};
			UpdateText();
			return;
		}

		if (m_hoveredEntity != previewRaycast->hitEntity)
		{
			m_hoveredEntity = previewRaycast->hitEntity;
			m_hoveredEntityPort = 0;
			UpdateText();
		}

		DrawEntityAABB(m_hoveredEntity, Nz::Color::Green());

		Nz::NodeComponent& entityNode = m_hoveredEntity.get<Nz::NodeComponent>();

		DistributionComponent& hoveredDistribution = m_hoveredEntity.get<DistributionComponent>();
		for (std::size_t inputIndex = 0; inputIndex < hoveredDistribution.GetInputCount(); ++inputIndex)
		{
			entt::handle inputEntity = hoveredDistribution.GetInputConnectedEntity(inputIndex);
			if (!inputEntity)
				continue;

			Nz::NodeComponent& inputEntityNode = inputEntity.get<Nz::NodeComponent>();

			DrawEntityAABB(inputEntity, Nz::Color::Red());
			debugDrawer->DrawLine(inputEntityNode.GetGlobalPosition(), entityNode.GetGlobalPosition(), s_distributionData[hoveredDistribution.GetInputType(inputIndex)].color);
		}

		for (std::size_t outputIndex = 0; outputIndex < hoveredDistribution.GetOutputCount(); ++outputIndex)
		{
			entt::handle outputEntity = hoveredDistribution.GetOutputConnectedEntity(outputIndex);
			if (!outputEntity)
				continue;

			Nz::NodeComponent& outputEntityNode = outputEntity.get<Nz::NodeComponent>();

			DrawEntityAABB(outputEntity, Nz::Color::Yellow());
			debugDrawer->DrawLine(entityNode.GetGlobalPosition(), outputEntityNode.GetGlobalPosition(), s_distributionData[hoveredDistribution.GetOutputType(outputIndex)].color);
		}
	}

	void ConnectTool::DrawEntityAABB(entt::handle entity, Nz::Color color)
	{
		if (Nz::GraphicsComponent* gfxComponent = entity.try_get<Nz::GraphicsComponent>())
		{
			Nz::NodeComponent& entityNode = entity.get<Nz::NodeComponent>();

			Nz::Boxf aabb = gfxComponent->GetAABB();
			aabb.ScaleAroundCenter(1.05f);

			auto aabbCorners = aabb.GetCorners();
			for (Nz::Vector3f& corner : aabbCorners)
				corner = entityNode.ToGlobalPosition(corner);

			Nz::DebugDrawer* debugDrawer = m_gameInterface.GetDebugDrawer();
			debugDrawer->DrawBoxCorners(aabbCorners, color);
		}
	}

	void ConnectTool::UpdateText()
	{
		m_textDrawer.Clear();

		Nz::RichTextBuilder textBuilder(m_textDrawer);

		if (m_selectedSourceEntity)
		{
			textBuilder << Nz::Color::White() << "Source entity";
			if (ClassInstanceComponent* entityClass = m_selectedSourceEntity.try_get<ClassInstanceComponent>())
				textBuilder << ": " << Nz::Color::Red() << entityClass->GetClass()->GetName() << Nz::Color::White();

			DistributionComponent& sourceDistribution = m_selectedSourceEntity.get<DistributionComponent>();
			if (m_sourceEntityPort < sourceDistribution.GetOutputCount())
				textBuilder << " (Output port " << s_distributionData[sourceDistribution.GetOutputType(m_sourceEntityPort)].color << std::to_string(m_sourceEntityPort) << Nz::Color::White() << ")";

			if (m_hoveredEntity && m_hoveredEntity != m_selectedSourceEntity)
			{
				textBuilder << Nz::Color::White() << "\n -> \nTarget entity";

				if (ClassInstanceComponent* entityClass = m_hoveredEntity.try_get<ClassInstanceComponent>())
					textBuilder << ": " << Nz::Color::Yellow() << entityClass->GetClass()->GetName() << Nz::Color::White();

				DistributionComponent& hoveredDistribution = m_hoveredEntity.get<DistributionComponent>();
				if (m_hoveredEntityPort < hoveredDistribution.GetInputCount())
					textBuilder << " (Input port " << s_distributionData[hoveredDistribution.GetInputType(m_hoveredEntityPort)].color << std::to_string(m_hoveredEntityPort) << Nz::Color::White() << ")";
			}
		}
		else if (m_hoveredEntity)
		{
			if (ClassInstanceComponent* entityClass = m_hoveredEntity.try_get<ClassInstanceComponent>())
				textBuilder << Nz::Color::White() << "Entity: " << Nz::Color::Red() << entityClass->GetClass()->GetName() << Nz::Color::White();
			else
				textBuilder << Nz::Color::White() << "Entity";

			DistributionComponent& hoveredDistribution = m_hoveredEntity.get<DistributionComponent>();
			if (m_hoveredEntityPort < hoveredDistribution.GetOutputCount())
				textBuilder << " (Output port " << s_distributionData[hoveredDistribution.GetOutputType(m_hoveredEntityPort)].color << std::to_string(m_hoveredEntityPort) << Nz::Color::White() << ")";
		}

		m_labelWidget->UpdateText(m_textDrawer);
		m_labelWidget->Center();
		m_labelWidget->SetPosition(m_labelWidget->GetPosition() + Nz::Vector2f(0.0f, -m_labelWidget->GetSize().y * 2.0f));
	}
}
