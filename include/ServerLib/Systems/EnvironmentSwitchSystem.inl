// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline EnvironmentSwitchSystem::EnvironmentSwitchSystem(entt::registry& registry, ServerEnvironment* ownerEnvironment) :
	m_registry(registry),
	m_ownerEnvironment(ownerEnvironment)
	{
	}
}
