// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/MenuState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/GameState.hpp>
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
		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		Nz::BoxLayout* addressLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* serverLabel = addressLayout->Add<Nz::LabelWidget>();
		serverLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Server: ", 24));

		m_serverAddressArea = addressLayout->Add<Nz::TextAreaWidget>();
		m_serverAddressArea->SetCharacterSize(24);
		m_serverAddressArea->SetText("malcolm.digitalpulse.software");
		m_serverAddressArea->SetTextColor(Nz::Color::Black());

		Nz::BoxLayout* loginLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* loginLabel = loginLayout->Add<Nz::LabelWidget>();
		loginLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Login: ", 24));

		m_loginArea = loginLayout->Add<Nz::TextAreaWidget>();
		m_loginArea->SetCharacterSize(24);
		m_loginArea->SetText("Mingebag");
		m_loginArea->SetTextColor(Nz::Color::Black());

		m_connectButton = m_layout->Add<Nz::ButtonWidget>();
		m_connectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Connect", 36, Nz::TextStyle_Regular, Nz::Color(0.13f)));
		m_connectButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			constexpr Nz::UInt16 GamePort = 29536;

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
			auto hostVec = Nz::IpAddress::ResolveHostname(Nz::NetProtocol::IPv6, m_serverAddressArea->GetText(), std::to_string(GamePort), &resolveError);

			if (hostVec.empty())
			{
				fmt::print(fg(fmt::color::red), "failed to resolve {}: {}\n", m_serverAddressArea->GetText(), Nz::ErrorToString(resolveError));
				return;
			}

			Nz::IpAddress serverAddress = hostVec[0].address;

			fmt::print("connecting to {}...\n", serverAddress.ToString());

			if (auto connectionState = m_connectionState.lock())
				connectionState->Connect(serverAddress, m_loginArea->GetText(), shared_from_this());
		});
	}

	bool MenuState::Update(Nz::StateMachine& fsm, Nz::Time /*elapsedTime*/)
	{
		/*if (auto connectionState = m_connectionState.lock())
		{
			if (connectionState->HasSession())
			{
				fsm.PopStatesUntil(shared_from_this());
				fsm.PopState();
			}
		}*/

		return true;
	}

	void MenuState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}
}
