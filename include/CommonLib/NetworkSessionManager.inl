// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <type_traits>

namespace tsom
{
	inline NetworkSessionManager::NetworkSessionManager(Nz::UInt16 port, Nz::NetProtocol protocol, std::size_t maxSessions) :
	m_sessions(maxSessions),
	m_handlerFactory(nullptr),
	m_reactor(0, protocol, port, maxSessions)
	{
	}

	template<typename T>
	void NetworkSessionManager::SetDefaultHandler()
	{
		static_assert(std::is_base_of_v<SessionHandler, T>);

		m_handlerFactory = []() -> std::unique_ptr<SessionHandler> { return std::make_unique<T>(); };
	}
}
