// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline entt::handle ClientEnvironmentHandler::GetControlledEntity() const
	{
		return m_playerControlledEntity;
	}

	inline const GravityController* ClientEnvironmentHandler::GetGravityController(std::size_t environmentIndex) const
	{
		if (environmentIndex > m_environments.size() || !m_environments[environmentIndex])
			return nullptr;

		return m_environments[environmentIndex]->gravityController;
	}

	inline ScriptingContext& ClientEnvironmentHandler::GetScriptingContext()
	{
		return m_scriptingContext;
	}
}
