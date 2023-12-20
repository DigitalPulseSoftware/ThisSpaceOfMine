// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/MenuState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/GameState.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Network/Algorithm.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <Nazara/Widgets.hpp>
#include <fmt/color.h>
#include <fmt/format.h>

namespace tsom
{
	MenuState::MenuState(std::shared_ptr<StateData> stateData, std::shared_ptr<ConnectionState> connectionState) :
	WidgetState(stateData),
	m_connectionState(connectionState)
	{
		std::string_view address = "malcolm.digitalpulse.software";
		std::string_view nickname = "Mingebag";

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

		m_connectButton = m_layout->Add<Nz::ButtonWidget>();
		m_connectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Connect", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_connectButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnConnectPressed();
		});

		m_autoConnect = cmdParams.HasFlag("auto-connect");
	}

	bool MenuState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_autoConnect)
		{
			OnConnectPressed();
			m_autoConnect = false;
		}

		return true;
	}

	void MenuState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->CenterHorizontal();
		m_layout->SetPosition(m_layout->GetPosition().x, newSize.y * 0.2f - m_layout->GetSize().y / 2.f);
	}

	void MenuState::OnConnectPressed()
	{
		if (m_serverAddressArea->GetText().empty())
		{
			fmt::print(fg(fmt::color::red), "missing server address\n");
			return;
		}

		if (m_loginArea->GetText().empty())
		{
			fmt::print(fg(fmt::color::red), "missing login\n");
			return;
		}

		Nz::ResolveError resolveError;
		auto hostVec = Nz::IpAddress::ResolveHostname(Nz::NetProtocol::Any, m_serverAddressArea->GetText(), std::to_string(Constants::ServerPort), &resolveError);

		if (hostVec.empty())
		{
			fmt::print(fg(fmt::color::red), "failed to resolve {}: {}\n", m_serverAddressArea->GetText(), Nz::ErrorToString(resolveError));
			return;
		}

		Nz::IpAddress serverAddress = hostVec[0].address;

		fmt::print("connecting to {}...\n", serverAddress.ToString());

		if (auto connectionState = m_connectionState.lock())
			connectionState->Connect(serverAddress, m_loginArea->GetText(), shared_from_this());
	}
}
