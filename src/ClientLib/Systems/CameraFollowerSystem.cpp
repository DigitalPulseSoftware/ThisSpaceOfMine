// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/CameraFollowerSystem.hpp>
#include <ClientLib/Components/CameraFollowerComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void CameraFollowerSystem::Update(Nz::Time /*elapsedTime*/)
	{
		auto view = m_registry.view<CameraFollowerComponent, Nz::NodeComponent>();
		for (auto&& [entity, followerComponent, nodeComponent] : view.each())
		{
			Nz::Vector3f currentPos = followerComponent.GetGlobalPosition();
			if (currentPos.SquaredDistance(m_cameraPosition) < Nz::IntegralPow(m_maxDistance, 2))
			{
				nodeComponent.CopyLocalTransform(followerComponent);
				continue;
			}

			float dist;
			Nz::Vector3f dir = (currentPos - m_cameraPosition).Normalize(&dist);

			Nz::Vector3f newPosition = m_cameraPosition + dir * m_maxDistance;
			float scale = m_maxDistance / dist;
			nodeComponent.SetTransform(newPosition, followerComponent.GetGlobalRotation(), scale * followerComponent.GetGlobalScale());
		}
	}
}
