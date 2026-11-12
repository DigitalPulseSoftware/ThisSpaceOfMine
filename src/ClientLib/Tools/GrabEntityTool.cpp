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
	void GrabEntityTool::OnTrigger(TriggerType triggerType)
	{
		if (triggerType != TriggerType::Primary)
			return;

		auto raycastHit = m_gameInterface.RaycastQuery();
		if (!raycastHit)
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
