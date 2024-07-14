// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/DirectConnectionState.hpp>
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
	DirectConnectionState::DirectConnectionState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState) :
	WidgetState(stateData),
	m_previousState(std::move(previousState))
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

		m_connectButton = m_layout->Add<Nz::ButtonWidget>();
		m_connectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Connect", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_connectButton->SetMaximumWidth(m_connectButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(m_connectButton->OnButtonTrigger, [this](const Nz::ButtonWidget*)
		{
			OnConnectPressed();
		});

		Nz::ButtonWidget* backButton = m_layout->Add<Nz::ButtonWidget>();
		backButton->UpdateText(Nz::SimpleTextDrawer::Draw("Back", 24, Nz::TextStyle_Regular, Nz::Color::Black()));
		backButton->SetMaximumWidth(backButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(backButton->OnButtonTrigger, [&](const Nz::ButtonWidget*)
		{
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
		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}

	void DirectConnectionState::OnConnectPressed()
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
		{
			Packets::AuthRequest::AnonymousPlayerData anonymousPlayer;
			anonymousPlayer.nickname = login;

			stateData.connectionState->Connect(serverAddress, std::move(anonymousPlayer), shared_from_this());
		}
	}
}
