// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <thread>

namespace tsom
{
	void ServerInstanceAppComponent::Update(Nz::Time elapsedTime)
	{
		Nz::Time nextUpdateTime = Nz::Time::Second();
		for (auto& instancePtr : m_instances)
			nextUpdateTime = std::min(nextUpdateTime, instancePtr->Update(elapsedTime));

		if (nextUpdateTime > Nz::Time::Milliseconds(5))
			std::this_thread::sleep_for(nextUpdateTime.AsDuration<std::chrono::milliseconds>());
	}
}
