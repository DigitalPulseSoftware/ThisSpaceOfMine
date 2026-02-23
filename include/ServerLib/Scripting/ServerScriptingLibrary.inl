// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ServerScriptingLibrary::ServerScriptingLibrary(ServerInstance& serverInstance, ServerEntityScriptingLibrary& entityScriptingLibrary) :
	m_entityScriptingLibrary(entityScriptingLibrary),
	m_serverInstance(serverInstance)
	{
		m_aliveSignal = std::make_shared<bool>(true);
	}

	inline ServerScriptingLibrary::~ServerScriptingLibrary()
	{
		*m_aliveSignal = false;
	}
}
