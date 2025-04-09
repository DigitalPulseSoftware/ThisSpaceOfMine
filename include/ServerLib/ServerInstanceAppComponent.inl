// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename... Args>
	ServerInstance& ServerInstanceAppComponent::AddInstance(Args&& ...args)
	{
		return *m_instances.emplace_back(std::make_unique<ServerInstance>(GetApp(), std::forward<Args>(args)...));
	}

	template<typename F>
	void ServerInstanceAppComponent::ForEachInstance(F&& callback)
	{
		for (auto& instancePtr : m_instances)
			callback(*instancePtr);
	}
}
