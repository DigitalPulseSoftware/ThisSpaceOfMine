// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/UpdateState.hpp>
#include <CommonLib/Utils.hpp>
#include <CommonLib/Version.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/Process.hpp>
#include <Nazara/Core/StateMachine.hpp>
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
			GetStateData().app->GetComponent<UpdaterAppComponent>().CancelUpdate();
			m_isCancelled = true;
		});
	}

	void UpdateState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		semver::version currentGameVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);

		auto& updater = GetStateData().app->GetComponent<UpdaterAppComponent>();
		m_onDownloadProgressSlot.Connect(updater.OnDownloadProgress, this, &UpdateState::UpdateProgressBar);
		m_onUpdateFailed.Connect(updater.OnUpdateFailed, [this]()
		{
			m_isCancelled = true;
		});

		updater.DownloadAndUpdate(m_updateInfo, m_updateInfo.assetVersion > currentGameVersion, m_updateInfo.binaryVersion > currentGameVersion);
	}

	bool UpdateState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_isCancelled)
			fsm.ChangeState(std::move(m_previousState));

		return true;
	}

	void UpdateState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize(newSize * 0.5f);
		m_layout->Center();

		if (m_progressionLabel)
			m_progressionLabel->Center();
	}

	void UpdateState::UpdateProgressBar(std::size_t activeDownloadCount, Nz::UInt64 downloaded, Nz::UInt64 total)
	{
		m_progressBar->SetFraction(float(downloaded) / float(total));

		m_progressionLabel->SetText(fmt::format("Downloading {0} file(s) - {1} / {2}", activeDownloadCount, ByteToString(downloaded), ByteToString(total)));
		m_progressionLabel->Resize(m_progressionLabel->GetPreferredSize());
		m_progressionLabel->Center();
	}
}
