// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ShipController::ShipController(entt::handle entity, const Nz::Quaternionf& rotation) :
	m_entity(entity),
	m_rotation(rotation)
	{
	}

	inline const Nz::Quaternionf& ShipController::GetReferenceRotation() const
	{
		return m_rotation;
	}
}
