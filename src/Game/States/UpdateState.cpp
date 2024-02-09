// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/UpdateState.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/Process.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/BoxLayout.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/ProgressBarWidget.hpp>
#include <Nazara/Widgets/SimpleLabelWidget.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <numeric>

namespace tsom
{
	namespace
	{
		std::string FormatSize(Nz::UInt64 sizeInBytes)
		{
			constexpr std::array<std::string_view, 7> s_units = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };

			std::size_t unitIndex = 0;
			for (; unitIndex < s_units.size(); ++unitIndex)
			{
				if (sizeInBytes < 1024 * 1024)
					break;

				sizeInBytes /= 1024;
			}

			double size = 0.0;
			if (sizeInBytes > 1024 && unitIndex < s_units.size() - 1)
			{
				size = sizeInBytes / 1024.0;
				unitIndex++;
			}
			else
				size = sizeInBytes;

			return fmt::format("{:.2f} {}", size, s_units[unitIndex]);
		}
	}

	UpdateState::UpdateState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState, UpdateInfo updateInfo) :
	WidgetState(stateData),
	m_previousState(std::move(previousState)),
	m_updateInfo(std::move(updateInfo)),
	m_isCancelled(false)
	{
		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		m_downloadLabel = m_layout->Add<Nz::SimpleLabelWidget>();
		m_downloadLabel->SetCharacterSize(24);
		m_downloadLabel->SetText("Downloading assets...");
		m_downloadLabel->Resize(m_downloadLabel->GetPreferredSize());
		m_downloadLabel->SetMaximumWidth(m_downloadLabel->GetWidth());

		m_progressBar = m_layout->Add<Nz::ProgressBarWidget>();
		m_progressBar->SetMaximumHeight(64.f);

		m_progressionLabel = m_progressBar->Add<Nz::SimpleLabelWidget>();
		m_progressionLabel->SetCharacterSize(24);
		m_progressionLabel->SetText("Starting download...");
		m_progressionLabel->Resize(m_progressionLabel->GetPreferredSize());

		// Set first download as the updater
		m_pendingDownloads.push_back({ "this_updater_of_mine", &m_updateInfo.updater, 0 });

		if (m_updateInfo.assets)
			m_pendingDownloads.push_back({ "autoupdate_assets", &m_updateInfo.assets.value(), 0 });

		m_pendingDownloads.push_back({ "autoupdate_binaries", &m_updateInfo.binaries, 0 });

		m_cancelButton = m_layout->Add<Nz::ButtonWidget>();
		m_cancelButton->UpdateText(Nz::SimpleTextDrawer::Draw("Cancel update", 24));
		m_cancelButton->SetMaximumSize(m_cancelButton->GetPreferredSize());

		m_cancelButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			m_isCancelled = true;
		});
	}

	void UpdateState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		m_hasUpdateStarted = false;
		for (auto& pendingDownload : m_pendingDownloads)
			StartDownload(pendingDownload);
	}

	bool UpdateState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		bool isDone = std::all_of(m_pendingDownloads.begin(), m_pendingDownloads.end(), [](const PendingDownload& pendingDownload)
		{
			return pendingDownload.isFinished;
		});

		if (isDone)
		{
			if (m_isCancelled)
			{
				fsm.ChangeState(std::move(m_previousState));
				return true;
			}

			if (!m_hasUpdateStarted)
			{
				m_hasUpdateStarted = true;
				StartUpdate();
			}

			return true;
		}

		return true;
	}

	void UpdateState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize(newSize * 0.5f);
		m_layout->Center();

		if (m_progressionLabel)
			m_progressionLabel->Center();
	}

	void UpdateState::StartDownload(PendingDownload& pendingDownload)
	{
		auto* webService = GetStateData().app->TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
		{
			// We shouldn't get in this state without web service, so this is just a fail-safe
			pendingDownload.isFinished = true;
			m_isCancelled = true;
			return;
		}

		std::filesystem::path path = Nz::Utf8Path(pendingDownload.name);
		path += Nz::Utf8Path(pendingDownload.info->downloadUrl).extension();

		pendingDownload.filename = Nz::PathToString(path);

		std::shared_ptr<Nz::File> file = std::make_shared<Nz::File>(path, Nz::OpenMode::Write);

		webService->QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupGet();
			request.SetURL(pendingDownload.info->downloadUrl);

			request.SetResultCallback([this, file, &pendingDownload](Nz::WebRequestResult&& result)
			{
				pendingDownload.isFinished = true;

				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to download {0}: {1}\n", pendingDownload.name, result.GetErrorMessage());
					file->Delete();
					return;
				}

				fmt::print(fg(fmt::color::green), "{} download succeeded!\n", pendingDownload.name);
			});

			request.SetDataCallback([this, file](const void* data, std::size_t length) mutable
			{
				if (file->Write(data, length) != length)
					return false;

				return true;
			});

			request.SetOptions(Nz::WebRequestOption::FailOnError | Nz::WebRequestOption::FollowRedirects);

			request.SetProgressCallback([this, &pendingDownload](std::size_t bytesReceived, std::size_t bytesTotal)
			{
				if (m_isCancelled)
					return false;

				if (bytesTotal != 0 && bytesTotal != pendingDownload.info->size)
				{
					fmt::print(fg(fmt::color::green), "error when downloading {0}: file size ({1}) doesn't match expected size ({2})!\n", pendingDownload.name, FormatSize(bytesTotal), FormatSize(pendingDownload.info->size));
					return false;
				}

				pendingDownload.downloadedSize = bytesReceived;
				UpdateProgressBar();

				return true;
			});

			return true;
		});
	}

	void UpdateState::StartUpdate()
	{
		std::filesystem::path updaterProgram = Nz::Utf8Path(m_pendingDownloads.front().name);

		Nz::Pid pid = Nz::Process::GetCurrentPid();

		std::vector<std::string> args;
		args.push_back(std::to_string(pid)); // pid to wait before starting update

		// TODO: Add a way to retrieve executable name (or use argv[0]?)
#ifdef NAZARA_PLATFORM_WINDOWS
		args.push_back("ThisSpaceOfMine.exe");
#else
		args.push_back("./ThisSpaceOfMine");
#endif

		// Skip first download (as it's the updater)
		for (std::size_t i = 1; i < m_pendingDownloads.size(); ++i)
			args.push_back(m_pendingDownloads[i].filename);

		Nz::Result updater = Nz::Process::SpawnDetached(updaterProgram, args);
		if (!updater)
		{
			fmt::print(fg(fmt::color::red), "failed to start autoupdater process: {0}\n", updater.GetError());
			return;
		}

		GetStateData().app->Quit();
	}

	void UpdateState::UpdateProgressBar()
	{
		std::size_t activeDownloadCount = 0;
		Nz::UInt64 totalDownloaded = 0;
		Nz::UInt64 totalSize = 0;

		for (auto& download : m_pendingDownloads)
		{
			totalDownloaded += download.downloadedSize;
			totalSize += download.info->size;

			if (download.downloadedSize != download.info->size)
				activeDownloadCount++;
		}

		m_progressBar->SetFraction(float(totalDownloaded) / float(totalSize));

		m_progressionLabel->SetText(fmt::format("Downloading {0} file(s) - {1} / {2}", activeDownloadCount, FormatSize(totalDownloaded), FormatSize(totalSize)));
		m_progressionLabel->Resize(m_progressionLabel->GetPreferredSize());
		m_progressionLabel->Center();
	}
}
