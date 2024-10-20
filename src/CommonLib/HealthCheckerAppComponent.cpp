// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/HealthCheckerAppComponent.hpp>
#include <fmt/color.h>
#include <chrono>
#include <csignal>

namespace tsom
{
	HealthCheckerAppComponent::HealthCheckerAppComponent(Nz::ApplicationBase& app, unsigned int maxHangSeconds) :
	ApplicationComponent(app),
	m_updateCounter(0),
	m_exitChecker(0),
	m_maxHangSeconds(maxHangSeconds)
	{
		m_healthCheckThread = std::thread([this]
		{
			unsigned int lastUpdateCounter = 0;
			unsigned int hangCounter = 0;

			while (!m_exitChecker.try_acquire_for(std::chrono::seconds(1)))
			{
				unsigned int updateCount = m_updateCounter.load(std::memory_order_relaxed);
				if NAZARA_LIKELY(updateCount != lastUpdateCounter)
				{
					hangCounter = 0;
					lastUpdateCounter = updateCount;
				}
				else
				{
					if (++hangCounter >= m_maxHangSeconds)
					{
						fmt::print(fg(fmt::color::red), "main loop has been unresponsive for {} seconds, exiting...", hangCounter);
						std::abort();
						break; //< just in case
					}
				}
			}

			// Normal exit
		});
	}

	HealthCheckerAppComponent::~HealthCheckerAppComponent()
	{
		m_exitChecker.release();
		m_healthCheckThread.join();
	}

	void HealthCheckerAppComponent::Update(Nz::Time elapsedTime)
	{
		m_updateCounter.fetch_add(1, std::memory_order_relaxed);
	}
}
