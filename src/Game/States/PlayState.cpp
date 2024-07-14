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

			FetchPlayerInfo();
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
			request.SetURL(fmt::format("{}/v1/player/auth", gameConfig.GetStringValue("Api.Url"), BuildConfig));
			request.SetServiceName("TSOM Player Info");

			nlohmann::json connectBody;
			connectBody["token"] = playerToken;

			request.SetJSonContent(connectBody.dump());

			request.SetResultCallback([widgetWeak = weak_from_this()](Nz::WebRequestResult&& result)
			{
				std::shared_ptr<PlayState> playState = std::static_pointer_cast<PlayState>(widgetWeak.lock());
				if (!playState)
					return;

				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to retrieve player info: {}\n", result.GetErrorMessage());
					playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to connect to server", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
					playState->m_createOrConnectButton->Disable();
					return;
				}

				if (result.GetStatusCode() != 200)
				{
					fmt::print(fg(fmt::color::red), "failed to retrieve player info (error {}): {}\n", result.GetStatusCode(), result.GetBody());
					playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to retrieve player", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
					playState->m_createOrConnectButton->Disable();
					return;
				}

				try
				{
					nlohmann::json responseDoc = nlohmann::json::parse(result.GetBody());

					std::string playerUuid = responseDoc["uuid"];
					std::string playerNickname = responseDoc["nickname"];

					playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw(fmt::format("Play as {}", playerNickname), 36, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
					playState->m_createOrConnectButton->Enable();
				}
				catch (const std::exception& e)
				{
					fmt::print(fg(fmt::color::red), "failed to retrieve player info (failed to decode response: {})\n", e.what());
					playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to retrieve player", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
					playState->m_createOrConnectButton->Disable();
					return;
				}
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
			auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();
			auto& webService = GetStateData().app->GetComponent<Nz::WebServiceAppComponent>();

			std::string_view playerToken = gameConfig.GetStringValue("Player.Token");

			webService.QueueRequest([&](Nz::WebRequest& request)
			{
				request.SetupPost();
				request.SetURL(fmt::format("{}/v1/game/connect", gameConfig.GetStringValue("Api.Url"), BuildConfig));
				request.SetServiceName("TSOM Player Info");

				nlohmann::json connectBody;
				connectBody["token"] = playerToken;

				request.SetJSonContent(connectBody.dump());

				request.SetResultCallback([widgetWeak = weak_from_this()](Nz::WebRequestResult&& result)
				{
					std::shared_ptr<PlayState> playState = std::static_pointer_cast<PlayState>(widgetWeak.lock());
					if (!playState)
						return;

					if (!result.HasSucceeded())
					{
						fmt::print(fg(fmt::color::red), "failed to retrieve player info: {}\n", result.GetErrorMessage());
						playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to connect to server", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
						return;
					}

					if (result.GetStatusCode() != 200)
					{
						fmt::print(fg(fmt::color::red), "failed to retrieve player info (error {}): {}\n", result.GetStatusCode(), result.GetBody());
						playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to retrieve connection token", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
						return;
					}

					try
					{
						nlohmann::json responseDoc = nlohmann::json::parse(result.GetBody());

						auto tokenResult = ConnectionToken::Deserialize(responseDoc);
						if (!tokenResult)
							throw std::runtime_error(tokenResult.GetError());

						ConnectionToken token = std::move(tokenResult).GetValue();

						Nz::ResolveError resolveError;
						auto hostVec = Nz::IpAddress::ResolveHostname(Nz::NetProtocol::Any, token.gameServer.address, std::to_string(token.gameServer.port), &resolveError);
						if (hostVec.empty())
						{
							fmt::print(fg(fmt::color::red), "failed to resolve {}:{}: {}\n", token.gameServer.address, token.gameServer.port, Nz::ErrorToString(resolveError));
							playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to resolve server address", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
							return;
						}

						Nz::IpAddress serverAddress = hostVec[0].address;

						auto& stateData = playState->GetStateData();
						if (stateData.connectionState)
						{
							Packets::AuthRequest::AuthenticatedPlayerData playerData;
							playerData.connectionToken = std::move(token);

							stateData.connectionState->Connect(serverAddress, std::move(playerData), playState);
						}
					}
					catch (const std::exception& e)
					{
						fmt::print(fg(fmt::color::red), "failed to retrieve player token (failed to decode response: {})\n", e.what());
						playState->m_createOrConnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Failed to retrieve connection token", 30, Nz::TextStyle_Regular, Nz::Color::Red()));
						return;
					}
				});

				return true;
			});
		}
		else
		{
			m_nextState = std::make_shared<CreatePlayerState>(GetStateDataPtr(), shared_from_this());
			return;
		}
	}
}
