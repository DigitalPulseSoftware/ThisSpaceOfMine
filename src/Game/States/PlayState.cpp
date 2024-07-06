// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/PlayState.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Version.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/CreatePlayerState.hpp>
#include <Game/States/DirectConnectionState.hpp>
#include <Game/States/GameState.hpp>
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
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>

namespace tsom
{
	PlayState::PlayState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState) :
	WidgetState(stateData),
	m_previousState(std::move(previousState))
	{
		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();

		std::string_view playerToken = gameConfig.GetStringValue("Player.Token");

		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		m_createOrConnectButton = m_layout->Add<Nz::ButtonWidget>();
		m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Loading player info...", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_createOrConnectButton->SetMaximumWidth(m_createOrConnectButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(m_createOrConnectButton->OnButtonTrigger, [this](const Nz::ButtonWidget*)
		{
			OnCreateOrConnectPressed();
		});

		m_directConnect = m_layout->Add<Nz::ButtonWidget>();
		m_directConnect->UpdateText(Nz::SimpleTextDrawer::Draw("Direct connection", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		m_directConnect->SetMaximumWidth(m_directConnect->GetPreferredWidth() * 1.5f);
		ConnectSignal(m_directConnect->OnButtonTrigger, [this](const Nz::ButtonWidget*)
		{
			m_nextState = std::make_shared<DirectConnectionState>(GetStateDataPtr(), shared_from_this());
		});

		Nz::ButtonWidget* backButton = m_layout->Add<Nz::ButtonWidget>();
		backButton->UpdateText(Nz::SimpleTextDrawer::Draw("Back", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
		backButton->SetMaximumWidth(backButton->GetPreferredWidth() * 1.5f);
		backButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			m_nextState = m_previousState;
		});

		const Nz::CommandLineParameters& cmdParams = GetStateData().app->GetCommandLineParameters();
		m_autoConnect = cmdParams.HasFlag("auto-connect");
	}

	void PlayState::Enter(Nz::StateMachine& fsm)
	{
		WidgetState::Enter(fsm);

		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();

		std::string_view playerToken = gameConfig.GetStringValue("Player.Token");

		if (!playerToken.empty())
		{
			m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Loading player info...", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
			m_createOrConnectButton->SetMaximumWidth(m_createOrConnectButton->GetPreferredWidth() * 1.5f);
			m_createOrConnectButton->Disable();
		}
		else
		{
			m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Create new player", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
			m_createOrConnectButton->SetMaximumWidth(m_createOrConnectButton->GetPreferredWidth() * 1.5f);
			m_createOrConnectButton->Enable();

			m_directConnect->UpdateText(Nz::SimpleTextDrawer::Draw("Play as guest", 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
			m_directConnect->SetMaximumWidth(m_directConnect->GetPreferredWidth() * 1.5f);
			m_directConnect->Enable();
		}

		// First layout happens in WidgetState::Enter and doesn't take m_createPlayerButton state into account
		LayoutWidgets(GetStateData().canvas->GetSize());

		FetchPlayerInfo();
	}

	bool PlayState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_nextState)
		{
			fsm.ChangeState(std::move(m_nextState));
			return true;
		}

		if (m_autoConnect)
		{
			OnCreateOrConnectPressed();
			m_autoConnect = false;
		}

		return true;
	}

	void PlayState::FetchPlayerInfo()
	{
		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();
		auto& webService = GetStateData().app->GetComponent<Nz::WebServiceAppComponent>();

		std::string_view playerToken = gameConfig.GetStringValue("Player.Token");

		webService.QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupPost();
			request.SetURL(fmt::format("{}/v1/player/authenticate", gameConfig.GetStringValue("Api.Url"), BuildConfig));
			request.SetServiceName("TSOM Player Info");

			nlohmann::json connectBody;
			connectBody["token"] = playerToken;

			request.SetJSonContent(connectBody.dump());

			request.SetResultCallback([&](Nz::WebRequestResult&& result)
			{
				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to retrieve player info: {}\n", result.GetErrorMessage());
					m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("failed to connect to server", 24, Nz::TextStyle::Bold, Nz::Color::Red()));
					return;
				}

				if (result.GetStatusCode() != 200)
				{
					fmt::print(fg(fmt::color::red), "failed to retrieve player info (error {}): {}\n", result.GetStatusCode(), result.GetBody());
					m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("failed to retrieve player", 24, Nz::TextStyle::Bold, Nz::Color::Red()));
					return;
				}

				nlohmann::json responseDoc = nlohmann::json::parse(result.GetBody());

				std::string playerGuid = responseDoc["guid"];
				std::string playerNickname = responseDoc["nickname"];

				m_directConnect->UpdateText(Nz::SimpleTextDrawer::Draw(fmt::format("Play as {}", playerNickname), 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
				m_directConnect->Enable();
			});

			return true;
		});
	}

	void PlayState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}

	void PlayState::OnCreateOrConnectPressed()
	{
		auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>();
		std::string_view playerToken = gameConfig.GetConfig().GetStringValue("Player.Token");

		if (!playerToken.empty())
		{
			// TODO
		}
		else
		{
			m_nextState = std::make_shared<CreatePlayerState>(GetStateDataPtr(), shared_from_this());
			return;
		}
	}
}
