// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename... Args>
	ServerWorld& ServerWorldAppComponent::AddWorld(Args&& ...args)
	{
		return *m_worlds.emplace_back(std::make_unique<ServerWorld>(std::forward<Args>(args)...));
	}
}
