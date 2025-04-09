// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVER_SERVERUPDATEAPPCOMPONENT_HPP
#define TSOM_SERVER_SERVERUPDATEAPPCOMPONENT_HPP

#include <CommonLib/UpdaterAppComponent.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>

namespace tsom
{
	class ServerUpdateAppComponent final : public Nz::ApplicationComponent
	{
		public:
			enum class State
			{
				WaitingForFetching,
				FetchingVersion,
				Downloading,
				WaitingForUpdate,
				Updating
			};

			enum class UpdateBehavior
			{
				DownloadAndExit,
				DownloadAndUpdate
			};

			inline ServerUpdateAppComponent(Nz::ApplicationBase& app, UpdateBehavior updateBehavior, Nz::Time checkInterval, Nz::Time quitDelay);
			ServerUpdateAppComponent(const ServerUpdateAppComponent&) = delete;
			ServerUpdateAppComponent(ServerUpdateAppComponent&&) = delete;
			~ServerUpdateAppComponent() = default;

			void Update(Nz::Time elapsedTime) override;

			ServerUpdateAppComponent& operator=(const ServerUpdateAppComponent&) = delete;
			ServerUpdateAppComponent& operator=(ServerUpdateAppComponent&&) = delete;

		private:
			NazaraSlot(UpdaterAppComponent, OnDownloadProgress, m_onDownloadProgress);
			NazaraSlot(UpdaterAppComponent, OnUpdateFailed, m_onUpdateFailed);
			NazaraSlot(UpdaterAppComponent, OnUpdateReady, m_onUpdateReady);

			UpdateBehavior m_updateBehavior;
			UpdateInfo m_updateInfo;
			Nz::Time m_checkInterval;
			Nz::Time m_quitDelay;
			Nz::Time m_waitingTime;
			State m_currentState;
	};
}

#include <Server/ServerUpdateAppComponent.inl>

#endif // TSOM_SERVER_SERVERUPDATEAPPCOMPONENT_HPP
