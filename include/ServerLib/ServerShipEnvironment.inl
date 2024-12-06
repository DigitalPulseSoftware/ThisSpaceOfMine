// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline entt::handle ServerShipEnvironment::GetExteriorShipEntity() const
	{
		return m_exteriorEntity;
	}

	inline const std::shared_ptr<Nz::Collider3D>& ServerShipEnvironment::GetInteriorAreaCollider() const
	{
		return m_interiorAreaColliders;
	}

	inline entt::handle ServerShipEnvironment::GetShipEntity() const
	{
		return m_shipEntity;
	}
}
