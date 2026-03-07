// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline CameraFollowerSystem::CameraFollowerSystem(entt::registry& registry, float maxDistance) :
	m_registry(registry),
	m_cameraPosition(Nz::Vector3f::Zero()),
	m_maxDistance(maxDistance)
	{
	}

	inline void CameraFollowerSystem::SetCameraPosition(const Nz::Vector3f& cameraPosition)
	{
		m_cameraPosition = cameraPosition;
	}
}
