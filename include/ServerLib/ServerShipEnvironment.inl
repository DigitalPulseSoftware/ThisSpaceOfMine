// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline entt::handle ServerShipEnvironment::GetOutsideShipEntity() const
	{
		return m_proxyEntity;
	}

	inline entt::handle ServerShipEnvironment::GetShipEntity() const
	{
		return m_shipEntity;
	}
}
