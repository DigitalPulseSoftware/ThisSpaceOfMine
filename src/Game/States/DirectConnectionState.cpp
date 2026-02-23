// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/DirectConnectionState.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Version.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/GameConfigs.hpp>
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
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <optional>

namespace tsom
{
	DirectConnectionState::DirectConnectionState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState) :
	WidgetState(stateData),
	m_previousState(std::move(previousState))
	{
		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();

		std::string_view address = gameConfig.GetStringValue(Config::Menu_ServerAddress);
		std::string_view nickname = gameConfig.GetStringValue(Config::Menu_Login);

		const Nz::CommandLineParameters& cmdParams = GetStateData().app->GetCommandLineParameters();
		cmdParams.GetParameter("server-address", &address);
		cmdParams.GetParameter("nickname", &nickname);

		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		Nz::BoxLayout* addressLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* serverLabel = addressLayout->Add<Nz::LabelWidget>();
		serverLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Server: ", 24));

		m_serverAddressArea = addressLayout->Add<Nz::TextAreaWidget>();
		m_serverAddressArea->EnableBackground(true);
		m_serverAddressArea->SetCharacterSize(24);
		m_serverAddressArea->SetText(std::string(address));
		m_serverAddressArea->SetTextColor(Nz::Color::Black());

		Nz::BoxLayout* loginLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* loginLabel = loginLayout->Add<Nz::LabelWidget>();
		loginLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Login: ", 24));

		m_loginArea = loginLayout->Add<Nz::TextAreaWidget>();
		m_loginArea->EnableBackground(true);
		m_loginArea->SetCharacterSize(24);
		m_loginArea->SetText(std::string(nickname));
		m_loginArea->SetTextColor(Nz::Color::Black());
		m_loginArea->SetMaximumTextLength(Constants::PlayerMaxNicknameLength);

		m_connectButton = m_layout->Add<Nz::ButtonWidget>();
		m_connectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Connect", 30, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_connectButton->SetMaximumWidth(m_connectButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(m_connectButton->OnButtonTrigger, [this](const Nz::ButtonWidget*)
		{
			OnConnectPressed();
		});

		Nz::ButtonWidget* backButton = m_layout->Add<Nz::ButtonWidget>();
		backButton->UpdateText(Nz::SimpleTextDrawer::Draw("Back", 30, Nz::TextStyle_Regular, Nz::Color::Black()));
		backButton->SetMaximumWidth(backButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(backButton->OnButtonTrigger, [&](const Nz::ButtonWidget*)
		{
			if (!m_nextState)
				m_nextState = m_previousState;
		});

		m_autoConnect = cmdParams.HasFlag("auto-connect");
	}

	bool DirectConnectionState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_autoConnect)
		{
			OnConnectPressed();
			m_autoConnect = false;
		}

		if (m_nextState)
		{
			fsm.ChangeState(std::move(m_nextState));
			return true;
		}

		return true;
	}

	void DirectConnectionState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ newSize.x * 0.33f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}

	void DirectConnectionState::OnConnectPressed()
	{
		if (m_serverAddressArea->GetText().empty())
		{
			spdlog::error("missing server address");
			return;
		}

		std::string login = std::string(Nz::Trim(m_loginArea->GetText(), Nz::UnicodeAware{}));
		if (login.empty())
		{
			spdlog::error("login cannot be blank");
			return;
		}

		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>();
		Nz::UInt16 serverPort = gameConfig.GetConfig().GetIntegerValue<Nz::UInt16>(Config::Server_Port);

		Nz::ResolveError resolveError;
		auto hostVec = Nz::IpAddress::ResolveHostname(Nz::NetProtocol::Any, m_serverAddressArea->GetText(), std::to_string(serverPort), &resolveError);

		if (hostVec.empty())
		{
			spdlog::error("failed to resolve {}: {}", m_serverAddressArea->GetText(), Nz::ErrorToString(resolveError));
			return;
		}

		gameConfig.GetConfig().SetStringValue(Config::Menu_Login, login);
		gameConfig.GetConfig().SetStringValue(Config::Menu_ServerAddress, m_serverAddressArea->GetText());

		gameConfig.Save();

		Nz::IpAddress serverAddress = hostVec[0].address;

		spdlog::info("connecting to {}...", serverAddress.ToString());

		auto& stateData = GetStateData();
		if (stateData.connectionState)
		{
			Packets::C_AuthRequest::AnonymousPlayerData anonymousPlayer;
			anonymousPlayer.nickname = login;

			stateData.connectionState->Connect(serverAddress, std::move(anonymousPlayer), shared_from_this());
		}
	}
}
