// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/VersionCheckState.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Version.hpp>
#include <Game/States/UpdateState.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Core/StringExt.hpp>
#include <Nazara/Network/Algorithm.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/BoxLayout.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace tsom
{
	VersionCheckState::VersionCheckState(std::shared_ptr<StateData> stateData) :
	WidgetState(stateData)
	{
		m_updateLayout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		m_updateLabel = m_updateLayout->Add<Nz::LabelWidget>();
		m_updateLabel->UpdateText(Nz::SimpleTextDrawer::Draw("A new version is available!", 18));

		m_updateButton = m_updateLayout->Add<Nz::ButtonWidget>();
		m_updateButton->UpdateText(Nz::SimpleTextDrawer::Draw("Update game", 18, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_updateButton->SetMaximumWidth(m_updateButton->GetPreferredWidth());
		m_updateButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnUpdatePressed();
		});
	}

	void VersionCheckState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		CheckVersion();
	}

	bool VersionCheckState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_nextState)
		{
			fsm.PopStatesUntil(shared_from_this());
			fsm.ChangeState(std::move(m_nextState));
			return true;
		}

		return true;
	}

	void VersionCheckState::CheckVersion()
	{
		m_updateLayout->Hide();
		m_newVersionInfo.reset();

		auto* updater = GetStateData().app->TryGetComponent<UpdaterAppComponent>();
		if (!updater)
			return;

		updater->FetchLastVersion([state = std::static_pointer_cast<VersionCheckState>(shared_from_this())](Nz::Result<UpdateInfo, std::string>&& result)
		{
			if (!result)
			{
				fmt::print(fg(fmt::color::red), "failed to get version update: {}\n", result.GetError());
				return;
			}

			state->OnUpdateInfoReceived(std::move(result).GetValue());
		});
	}

	void VersionCheckState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_updateLayout->Resize({ std::max(m_updateLabel->GetPreferredWidth(), m_updateButton->GetPreferredWidth()), m_updateLabel->GetPreferredHeight() * 2.f + m_updateButton->GetPreferredHeight() });
		m_updateLayout->SetPosition(newSize * Nz::Vector2f(0.9f, 0.1f) - m_updateButton->GetSize() * 0.5f);
	}

	void VersionCheckState::OnUpdateInfoReceived(UpdateInfo&& updateInfo)
	{
		semver::version currentGameVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);

		if (updateInfo.assetVersion > currentGameVersion || updateInfo.binaryVersion > currentGameVersion)
		{
			m_newVersionInfo = std::move(updateInfo);
			fmt::print(fg(fmt::color::yellow), "new version available: {}\n", m_newVersionInfo->binaryVersion.to_string());

			// We're not supposed to be able to have asset-only version but let's prepare for this
			semver::version biggestVer = std::max(m_newVersionInfo->assetVersion, m_newVersionInfo->binaryVersion);

			m_updateButton->UpdateText(Nz::SimpleTextDrawer::Draw("Update game to " + biggestVer.to_string(), 18, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
			m_updateButton->SetMaximumWidth(m_updateButton->GetPreferredWidth());

			m_updateLayout->Show();
		}
		else
			fmt::print("no new version available\n");
	}

	void VersionCheckState::OnUpdatePressed()
	{
		if (!m_newVersionInfo)
			return;

		m_nextState = std::make_shared<UpdateState>(GetStateDataPtr(), shared_from_this(), std::move(*m_newVersionInfo));
		m_newVersionInfo.reset();
	}
}
