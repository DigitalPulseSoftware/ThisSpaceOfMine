// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Server/ServerUpdateAppComponent.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <CommonLib/Utils.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <fmt/color.h>
#include <fmt/std.h>

namespace tsom
{
	void ServerUpdateAppComponent::Update(Nz::Time elapsedTime)
	{
		if (m_currentState != State::WaitingForFetching &&
			m_currentState != State::WaitingForUpdate)
			return;

		m_waitingTime -= elapsedTime;
		if (m_waitingTime >= Nz::Time::Zero())
			return;

		if (m_currentState == State::WaitingForFetching)
		{
			m_currentState = State::FetchingVersion;

			auto& appUpdater = GetApp().GetComponent<UpdaterAppComponent>();
			appUpdater.FetchLastVersion(true, [this, &appUpdater](Nz::Result<UpdateInfo, std::string>&& result)
			{
				if (!result)
				{
					fmt::print(fg(fmt::color::dark_green), "failed to get last server version: {}\n", result.GetError());
					m_currentState = State::WaitingForFetching;
					m_waitingTime = m_checkInterval;
					return;
				}

				m_updateInfo = result.GetValue();

				semver::version currentGameVersion(GameMajorVersion, GameMinorVersion, 1);
				if (m_updateInfo.assetVersion == currentGameVersion && m_updateInfo.binaryVersion == currentGameVersion)
				{
					m_currentState = State::WaitingForFetching;
					m_waitingTime = m_checkInterval;
					return; // no version available
				}

				// Download without updating
				m_onUpdateFailed.Connect(appUpdater.OnUpdateFailed, [this]
				{
					m_currentState = State::WaitingForFetching;
					m_waitingTime = m_checkInterval;
				});

				m_onDownloadProgress.Connect(appUpdater.OnDownloadProgress, [lastPrint = Nz::MillisecondClock()](std::size_t activeDownloadCount, Nz::UInt64 downloaded, Nz::UInt64 total) mutable
				{
					if (lastPrint.RestartIfOver(Nz::Time::Second()))
						fmt::print("downloading {} file(s) ({}/{}) - {}%\n", activeDownloadCount, ByteToString(downloaded), ByteToString(total), 100 * downloaded / total);
				});

				m_onUpdateReady.Connect(appUpdater.OnUpdateReady, [this]
				{
					m_currentState = State::WaitingForUpdate;
					m_waitingTime = m_quitDelay;

					std::string message;
					if (m_quitDelay > Nz::Time::Zero())
						message = fmt::format("A new server version has been downloaded ({}), restarting in {}...", m_updateInfo.binaryVersion.to_string(), fmt::streamed(m_quitDelay));
					else
						message = fmt::format("A new server version has been downloaded ({}), restarting...", m_updateInfo.binaryVersion.to_string());

					fmt::print(fg(fmt::color::dark_green), "{}\n", message);

					auto& serverInstance = GetApp().GetComponent<ServerInstanceAppComponent>();
					serverInstance.ForEachInstance([&](ServerInstance& serverInstance)
					{
						serverInstance.BroadcastChatMessage(message);
					});
				});

				fmt::print(fg(fmt::color::dark_green), "A new server version has been found ({}), downloading...\n", m_updateInfo.binaryVersion.to_string());

				appUpdater.DownloadAndUpdate(m_updateInfo, m_updateInfo.assetVersion > currentGameVersion, m_updateInfo.binaryVersion > currentGameVersion, m_updateBehavior == UpdateBehavior::DownloadAndUpdate, false);
			});
		}
		else if (m_currentState == State::WaitingForUpdate)
		{
			m_currentState = State::Updating;

			semver::version currentGameVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);

			auto& appUpdater = GetApp().GetComponent<UpdaterAppComponent>();
			
			auto& serverInstance = GetApp().GetComponent<ServerInstanceAppComponent>();
			serverInstance.ForEachInstance([&](ServerInstance& serverInstance)
			{
				serverInstance.BroadcastChatMessage("Restarting for server update...");
			});

			fmt::print(fg(fmt::color::dark_green), "Restarting for server update...\n");

			// Call DownloadAndUpdate again, which should validate already downloaded files and exit the application
			appUpdater.DownloadAndUpdate(m_updateInfo, m_updateInfo.assetVersion > currentGameVersion, m_updateInfo.binaryVersion > currentGameVersion, m_updateBehavior == UpdateBehavior::DownloadAndUpdate, true);
		}
	}
}
