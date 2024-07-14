// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
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
#include <Game/States/PlayState.hpp>
#include <Game/States/UpdateState.hpp>
#include <Nazara/Widgets.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Core/StringExt.hpp>
#include <Nazara/Graphics/PredefinedMaterials.hpp>
#include <Nazara/Network/Algorithm.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>

namespace tsom
{
	MenuState::MenuState(std::shared_ptr<StateData> stateData) :
	WidgetState(stateData),
	m_accumulator(Nz::Time::Zero())
	{
		m_logo = CreateWidget<Nz::SimpleLabelWidget>();

		m_logo->UpdateDrawer([&](Nz::SimpleTextDrawer& textDrawer)
		{
			auto& filesystem = GetStateData().app->GetComponent<Nz::FilesystemAppComponent>();

			textDrawer.SetTextOutlineColor(Nz::Color::White());
			textDrawer.SetText("This Space Of Mine");
			textDrawer.SetTextFont(filesystem.Open<Nz::Font>("assets/fonts/axaxax bd.otf"));
			textDrawer.SetTextStyle(Nz::TextStyle::OutlineOnly);
		});

		m_logoBackground = m_logo->Add<Nz::SimpleLabelWidget>();
		m_logoBackground->UpdateDrawer([&](Nz::SimpleTextDrawer& textDrawer)
		{
			auto& filesystem = GetStateData().app->GetComponent<Nz::FilesystemAppComponent>();

			textDrawer.SetTextOutlineColor(Nz::Color(1.f, 1.f, 1.f, 0.3f));
			textDrawer.SetText("This Space Of Mine");
			textDrawer.SetTextFont(filesystem.Open<Nz::Font>("assets/fonts/axaxax bd.otf"));
			textDrawer.SetTextStyle(Nz::TextStyle::OutlineOnly);
		});
		m_logoBackground->SetPosition({ 7.f, -7.f });

		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		m_playButton = m_layout->Add<Nz::ButtonWidget>();
		m_playButton->UpdateText(Nz::SimpleTextDrawer::Draw("Play", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_playButton->SetMaximumWidth(m_playButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(m_playButton->OnButtonTrigger, [this](const Nz::ButtonWidget*)
		{
			m_nextState = std::make_shared<PlayState>(GetStateDataPtr(), shared_from_this());
		});

		m_quitGameButton = m_layout->Add<Nz::ButtonWidget>();
		m_quitGameButton->UpdateText(Nz::SimpleTextDrawer::Draw("Quit", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_quitGameButton->SetMaximumWidth(m_quitGameButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(m_quitGameButton->OnButtonTrigger, [this](const Nz::ButtonWidget*)
		{
			GetStateData().app->Quit();
		});

		const Nz::CommandLineParameters& cmdParams = GetStateData().app->GetCommandLineParameters();
		m_autoConnect = cmdParams.HasFlag("auto-connect");
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
			m_autoConnect = false;
		}

		m_accumulator += elapsedTime;
		m_logo->UpdateDrawer([&](Nz::SimpleTextDrawer& textDrawer)
		{
			textDrawer.SetCharacterSpacingOffset(std::sin(m_accumulator.AsSeconds() * 0.2f) * 5.f);
		});
		m_logo->CenterHorizontal();

		m_logoBackground->UpdateDrawer([&](Nz::SimpleTextDrawer& textDrawer)
		{
			textDrawer.SetCharacterSpacingOffset(std::sin(m_accumulator.AsSeconds() * 0.2f) * 5.f);
		});

		return true;
	}

	void MenuState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_logo->UpdateDrawer([&](Nz::SimpleTextDrawer& textDrawer)
		{
			textDrawer.SetCharacterSize(static_cast<unsigned int>(newSize.y / 7.5f));
			textDrawer.SetTextOutlineThickness(std::floor(newSize.y / 360.f));
		});

		m_logoBackground->UpdateDrawer([&](Nz::SimpleTextDrawer& textDrawer)
		{
			textDrawer.SetCharacterSize(static_cast<unsigned int>(newSize.y / 7.5f));
			textDrawer.SetTextOutlineThickness(std::floor(newSize.y / 300.f));
		});

		m_logo->SetPosition({ 0.f, newSize.y * 0.66f });
		m_logo->CenterHorizontal();

		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}
}
