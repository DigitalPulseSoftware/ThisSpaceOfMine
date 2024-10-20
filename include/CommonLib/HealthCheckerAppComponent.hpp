// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_HEALTHCHECKERAPPCOMPONENT_HPP
#define TSOM_COMMONLIB_HEALTHCHECKERAPPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <atomic>
#include <semaphore>
#include <thread>

namespace tsom
{
	class TSOM_COMMONLIB_API HealthCheckerAppComponent : public Nz::ApplicationComponent
	{
		public:
			HealthCheckerAppComponent(Nz::ApplicationBase& app, unsigned int maxHangSeconds);
			HealthCheckerAppComponent(const HealthCheckerAppComponent&) = delete;
			HealthCheckerAppComponent(HealthCheckerAppComponent&&) = delete;
			~HealthCheckerAppComponent();

			HealthCheckerAppComponent& operator=(const HealthCheckerAppComponent&) = delete;
			HealthCheckerAppComponent& operator=(HealthCheckerAppComponent&&) = delete;

		private:
			void Update(Nz::Time elapsedTime) override;

			std::atomic_uint m_updateCounter;
			std::binary_semaphore m_exitChecker;
			std::thread m_healthCheckThread;
			unsigned int m_maxHangSeconds;
	};
}

#include <CommonLib/HealthCheckerAppComponent.inl>

#endif // TSOM_COMMONLIB_HEALTHCHECKERAPPCOMPONENT_HPP
