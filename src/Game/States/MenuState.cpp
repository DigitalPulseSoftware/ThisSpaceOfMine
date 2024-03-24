// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/MenuState.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Version.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/GameState.hpp>
#include <Game/States/UpdateState.hpp>
#include <Nazara/Widgets.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Core/StringExt.hpp>
#include <Nazara/Network/Algorithm.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>

namespace tsom
{
	MenuState::MenuState(std::shared_ptr<StateData> stateData) :
	WidgetState(stateData)
	{
		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();

		std::string_view address = gameConfig.GetStringValue("Menu.ServerAddress");
		std::string_view nickname = gameConfig.GetStringValue("Menu.Login");

		const Nz::CommandLineParameters& cmdParams = GetStateData().app->GetCommandLineParameters();
		cmdParams.GetParameter("server-address", &address);
		cmdParams.GetParameter("nickname", &nickname);

		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		Nz::BoxLayout* addressLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* serverLabel = addressLayout->Add<Nz::LabelWidget>();
		serverLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Server: ", 24));

		m_serverAddressArea = addressLayout->Add<Nz::TextAreaWidget>();
		m_serverAddressArea->SetCharacterSize(24);
		m_serverAddressArea->SetText(std::string(address));
		m_serverAddressArea->SetTextColor(Nz::Color::Black());

		Nz::BoxLayout* loginLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* loginLabel = loginLayout->Add<Nz::LabelWidget>();
		loginLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Login: ", 24));

		m_loginArea = loginLayout->Add<Nz::TextAreaWidget>();
		m_loginArea->SetCharacterSize(24);
		m_loginArea->SetText(std::string(nickname));
		m_loginArea->SetTextColor(Nz::Color::Black());
		m_loginArea->SetMaximumTextLength(Constants::PlayerMaxNicknameLength);

		m_connectButton = m_layout->Add<Nz::ButtonWidget>();
		m_connectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Connect", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_connectButton->SetMaximumWidth(m_connectButton->GetPreferredWidth() * 1.5f);
		m_connectButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnConnectPressed();
		});

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

		m_autoConnect = cmdParams.HasFlag("auto-connect");
	}

	void MenuState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		CheckVersion();
	}

	bool MenuState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_nextState)
		{
			fsm.ChangeState(std::move(m_nextState));
			return true;
		}

		if (m_autoConnect)
		{
			OnConnectPressed();
			m_autoConnect = false;
		}

		return true;
	}

	void MenuState::CheckVersion()
	{
		m_updateLayout->Hide();
		m_newVersionInfo.reset();

		auto* updater = GetStateData().app->TryGetComponent<UpdaterAppComponent>();
		if (!updater)
			return;

		updater->FetchLastVersion([state = std::static_pointer_cast<MenuState>(shared_from_this())](Nz::Result<UpdateInfo, std::string>&& result)
		{
			if (!result)
			{
				fmt::print(fg(fmt::color::red), "failed to get version update: {}\n", result.GetError());
				return;
			}

			state->OnUpdateInfoReceived(std::move(result).GetValue());
		});
	}

	void MenuState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->CenterHorizontal();
		m_layout->SetPosition({ m_layout->GetPosition().x, newSize.y * 0.2f - m_layout->GetSize().y / 2.f });

		m_updateLayout->Resize({ std::max(m_updateLabel->GetPreferredWidth(), m_updateButton->GetPreferredWidth()), m_updateLabel->GetPreferredHeight() * 2.f + m_updateButton->GetPreferredHeight() });
		m_updateLayout->SetPosition(newSize * Nz::Vector2f(0.9f, 0.1f) - m_updateButton->GetSize() * 0.5f);
	}

	void MenuState::OnConnectPressed()
	{
		if (m_serverAddressArea->GetText().empty())
		{
			fmt::print(fg(fmt::color::red), "missing server address\n");
			return;
		}

		std::string login = std::string(Nz::Trim(m_loginArea->GetText(), Nz::UnicodeAware{}));
		if (login.empty())
		{
			fmt::print(fg(fmt::color::red), "login cannot be blank\n");
			return;
		}

		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>();
		Nz::UInt16 serverPort = gameConfig.GetConfig().GetIntegerValue<Nz::UInt16>("Server.Port");

		Nz::ResolveError resolveError;
		auto hostVec = Nz::IpAddress::ResolveHostname(Nz::NetProtocol::Any, m_serverAddressArea->GetText(), std::to_string(serverPort), &resolveError);

		if (hostVec.empty())
		{
			fmt::print(fg(fmt::color::red), "failed to resolve {}: {}\n", m_serverAddressArea->GetText(), Nz::ErrorToString(resolveError));
			return;
		}

		gameConfig.GetConfig().SetStringValue("Menu.Login", login);
		gameConfig.GetConfig().SetStringValue("Menu.ServerAddress", m_serverAddressArea->GetText());

		gameConfig.Save();

		Nz::IpAddress serverAddress = hostVec[0].address;

		fmt::print("connecting to {}...\n", serverAddress.ToString());

		auto& stateData = GetStateData();
		if (stateData.connectionState)
			stateData.connectionState->Connect(serverAddress, login, shared_from_this());
	}

	void MenuState::OnUpdateInfoReceived(UpdateInfo&& updateInfo)
	{
		semver::version currentGameVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);

		if (updateInfo.assetVersion > currentGameVersion || updateInfo.binaryVersion > currentGameVersion)
		{
			fmt::print(fg(fmt::color::yellow), "new version available: {}\n", m_newVersionInfo->binaryVersion.to_string());
			m_newVersionInfo = std::move(updateInfo);

			// We're not supposed to be able to have asset-only version but let's prepare for this
			semver::version biggestVer = std::max(updateInfo.assetVersion, updateInfo.binaryVersion);

			m_updateButton->UpdateText(Nz::SimpleTextDrawer::Draw("Update game to " + biggestVer.to_string(), 18, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
			m_updateButton->SetMaximumWidth(m_updateButton->GetPreferredWidth());

			m_updateLayout->Show();
		}
		else
			fmt::print("no new version available\n");
	}

	void MenuState::OnUpdatePressed()
	{
		if (!m_newVersionInfo)
			return;

		m_nextState = std::make_shared<UpdateState>(GetStateDataPtr(), shared_from_this(), std::move(*m_newVersionInfo));
		m_newVersionInfo.reset();
	}
}
