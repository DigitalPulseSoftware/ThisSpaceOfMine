// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename... Args>
	ServerInstance& ServerInstanceAppComponent::AddInstance(Args&& ...args)
	{
		return *m_instances.emplace_back(std::make_unique<ServerInstance>(std::forward<Args>(args)...));
	}
}
