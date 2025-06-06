// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ClientScriptingLibrary::ClientScriptingLibrary(Nz::ApplicationBase& app, ClientEnvironmentHandler& clientEnvironmentHandler, ClientSessionHandler& sessionHandler) :
	m_app(app),
	m_environmentHandler(clientEnvironmentHandler),
	m_sessionHandler(sessionHandler)
	{
	}
}
