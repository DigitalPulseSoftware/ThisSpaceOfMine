// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ServerUpdateAppComponent::ServerUpdateAppComponent(Nz::ApplicationBase& app, UpdateBehavior updateBehavior, Nz::Time checkInterval, Nz::Time quitDelay) :
	ApplicationComponent(app),
	m_updateBehavior(updateBehavior),
	m_checkInterval(checkInterval),
	m_quitDelay(quitDelay),
	m_waitingTime(m_checkInterval),
	m_currentState(State::WaitingForFetching)
	{
	}
}
