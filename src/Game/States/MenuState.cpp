// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/MenuState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/GameState.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Utility/SimpleTextDrawer.hpp>
#include <Nazara/Widgets.hpp>

namespace tsom
{
	MenuState::MenuState(std::shared_ptr<StateData> stateData, std::shared_ptr<ConnectionState> connectionState) :
	WidgetState(stateData),
	m_connectionState(connectionState)
	{
		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		Nz::BoxLayout* loginLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::LabelWidget* loginLabel = loginLayout->Add<Nz::LabelWidget>();
		loginLabel->UpdateText(Nz::SimpleTextDrawer::Draw("Login: ", 24));

		m_loginArea = loginLayout->Add<Nz::TextAreaWidget>();
		m_loginArea->SetCharacterSize(24);
		m_loginArea->SetTextColor(Nz::Color::Black());

		m_connectButton = m_layout->Add<Nz::ButtonWidget>();
		m_connectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Connect", 36, Nz::TextStyle_Regular, Nz::Color(0.13f)));
		m_connectButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			std::shared_ptr<Nz::State> gameState = std::make_shared<GameState>(GetStateDataPtr());

			Nz::IpAddress local = Nz::IpAddress::LoopbackIpV6;
			local.SetPort(29536);

			if (auto connectionState = m_connectionState.lock())
				connectionState->Connect(local, m_loginArea->GetText(), shared_from_this(), gameState);
		});
	}

	bool MenuState::Update(Nz::StateMachine& fsm, Nz::Time /*elapsedTime*/)
	{
		if (auto connectionState = m_connectionState.lock())
		{
			if (connectionState->HasSession())
			{
				fsm.PopStatesUntil(shared_from_this());
				fsm.PopState();
			}
		}

		return true;
	}

	void MenuState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ 256.f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}
}
