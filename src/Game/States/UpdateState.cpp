// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/UpdateState.hpp>
#include <CommonLib/Utils.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/Process.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/BoxLayout.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/ProgressBarWidget.hpp>
#include <Nazara/Widgets/SimpleLabelWidget.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <numeric>

namespace tsom
{
	UpdateState::UpdateState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState, UpdateInfo updateInfo) :
	WidgetState(stateData),
	m_previousState(std::move(previousState)),
	m_downloadManager(*GetStateData().app),
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

		m_cancelButton = m_layout->Add<Nz::ButtonWidget>();
		m_cancelButton->UpdateText(Nz::SimpleTextDrawer::Draw("Cancel update", 24));
		m_cancelButton->SetMaximumSize(m_cancelButton->GetPreferredSize());

		m_cancelButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			m_downloadManager.Cancel();
		});
	}

	void UpdateState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		m_hasUpdateStarted = false;

		m_activeDownloads.clear(); //< just in case
		auto QueueDownload = [&](std::string_view filename, const UpdateInfo::DownloadInfo* info)
		{
			auto download = m_downloadManager.QueueDownload(Nz::Utf8Path(filename), info->downloadUrl, info->size, info->sha256);
			m_activeDownloads.push_back(download);

			return download;
		};

		if (m_updateInfo.assets)
			m_updateArchives.push_back(QueueDownload("autoupdate_assets", &m_updateInfo.assets.value()));

		m_updateArchives.push_back(QueueDownload("autoupdate_binaries", &m_updateInfo.binaries));

		m_updaterDownload = QueueDownload("this_updater_of_mine", &m_updateInfo.updater);

		for (auto& downloadPtr : m_activeDownloads)
		{
			downloadPtr->OnDownloadProgress.Connect([this](const DownloadManager::Download&)
			{
				UpdateProgressBar();
			});
		}
	}

	bool UpdateState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (!m_downloadManager.HasDownloadInProgress())
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

	void UpdateState::StartUpdate()
	{
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
		for (auto& downloadPtr : m_updateArchives)
			args.push_back(Nz::PathToString(downloadPtr->filepath));

		Nz::Result updater = Nz::Process::SpawnDetached(m_updaterDownload->filepath, args);
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

		for (auto& downloadPtr : m_activeDownloads)
		{
			totalDownloaded += downloadPtr->downloadedSize;
			totalSize += downloadPtr->totalSize;

			if (downloadPtr->downloadedSize != downloadPtr->totalSize)
				activeDownloadCount++;
		}

		m_progressBar->SetFraction(float(totalDownloaded) / float(totalSize));

		m_progressionLabel->SetText(fmt::format("Downloading {0} file(s) - {1} / {2}", activeDownloadCount, FormatSize(totalDownloaded), FormatSize(totalSize)));
		m_progressionLabel->Resize(m_progressionLabel->GetPreferredSize());
		m_progressionLabel->Center();
	}
}
