// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <NazaraUtils/Algorithm.hpp>

namespace tsom
{
	inline SessionVisibilityHandler::SessionVisibilityHandler(NetworkSession* networkSession) :
	m_networkSession(networkSession)
	{
	}

	inline std::size_t SessionVisibilityHandler::HandlerHasher::operator()(const entt::handle& handle) const
	{
		std::size_t seed = std::hash<entt::registry*>{}(handle.registry());
		Nz::HashCombine(seed, handle.entity());
		return seed;
	}
}
