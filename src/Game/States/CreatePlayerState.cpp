// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Game/States/CreatePlayerState.hpp>
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
	CreatePlayerState::CreatePlayerState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState) :
	WidgetState(stateData),
	m_previousState(std::move(previousState)),
	m_nextStateTimer(Nz::Time::Zero())
	{
		m_status = CreateWidget<Nz::LabelWidget>();

		m_layout = CreateWidget<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		Nz::BoxLayout* nicknameLayout = m_layout->Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::LeftToRight);

		Nz::SimpleLabelWidget* nicknameLabel = nicknameLayout->Add<Nz::SimpleLabelWidget>();
		nicknameLabel->SetText("Nickname:");
		nicknameLabel->SetMaximumWidth(nicknameLabel->GetPreferredWidth());

		Nz::TextAreaWidget* nicknameText = nicknameLayout->Add<Nz::TextAreaWidget>();
		nicknameText->EnableBackground(true);
		nicknameText->SetCharacterSize(24);
		nicknameText->SetTextColor(Nz::Color::Black());

		Nz::ButtonWidget* createButton = m_layout->Add<Nz::ButtonWidget>();
		createButton->UpdateText(Nz::SimpleTextDrawer::Draw("Create player", 24, Nz::TextStyle_Regular, Nz::Color::Black()));
		createButton->SetMaximumWidth(createButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(createButton->OnButtonTrigger, [this, nicknameText](const Nz::ButtonWidget* /*button*/)
		{
			OnCreatePressed(nicknameText->GetText());
		});

		Nz::ButtonWidget* backButton = m_layout->Add<Nz::ButtonWidget>();
		backButton->UpdateText(Nz::SimpleTextDrawer::Draw("Back", 24, Nz::TextStyle_Regular, Nz::Color::Black()));
		backButton->SetMaximumWidth(backButton->GetPreferredWidth() * 1.5f);
		ConnectSignal(backButton->OnButtonTrigger, [&](const Nz::ButtonWidget* /*button*/)
		{
			m_nextState = m_previousState;
		});
	}

	bool CreatePlayerState::Update(Nz::StateMachine& fsm, Nz::Time elapsedTime)
	{
		if (m_nextState)
		{
			m_nextStateTimer -= elapsedTime;
			if (m_nextStateTimer <= Nz::Time::Zero())
			{
				fsm.ChangeState(std::move(m_nextState));
				return true;
			}
		}

		return true;
	}

	void CreatePlayerState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_status->SetPosition(newSize * Nz::Vector2f(0.5f, 0.8f) - m_status->GetSize() * 0.5f);

		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->Center();
	}

	void CreatePlayerState::OnCreatePressed(const std::string& nickname)
	{
		auto& gameConfigComponent = GetStateData().app->GetComponent<GameConfigAppComponent>();
		auto& gameConfig = gameConfigComponent.GetConfig();
		auto& webService = GetStateData().app->GetComponent<Nz::WebServiceAppComponent>();

		webService.QueueRequest([&, nickname](Nz::WebRequest& request) mutable
		{
			request.SetupPost();
			request.SetURL(fmt::format("{}/v1/players", gameConfig.GetStringValue("Api.Url"), BuildConfig));
			request.SetServiceName("TSOM Player Create");

			nlohmann::json createParams;
			createParams["nickname"] = nickname;

			request.SetJSonContent(createParams.dump());

			request.SetResultCallback([&, nickname = std::move(nickname)](Nz::WebRequestResult&& result)
			{
				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to create player: {}\n", result.GetErrorMessage());
					return;
				}

				if (result.GetStatusCode() != 200)
				{
					fmt::print(fg(fmt::color::red), "failed to create player (error {})\n", result.GetStatusCode());

					try
					{
						nlohmann::json error = nlohmann::json::parse(result.GetBody());

						std::string_view errorMessage = error["err_desc"];
						UpdateStatus(Nz::SimpleTextDrawer::Draw(fmt::format("Failed to create player: {}", errorMessage), 36, Nz::TextStyle_Regular, Nz::Color::Red()));
					}
					catch (const std::exception& e)
					{
						fmt::print(fg(fmt::color::red), "failed to decode error from API: {}\n", e.what());
						UpdateStatus(Nz::SimpleTextDrawer::Draw("Failed to create player", 36, Nz::TextStyle_Regular, Nz::Color::Red()));
					}

					return;
				}

				nlohmann::json responseDoc = nlohmann::json::parse(result.GetBody());

				std::string_view playerGuid = responseDoc["guid"];
				std::string_view connectToken = responseDoc["token"];

				gameConfig.SetStringValue("Player.Token", std::string(connectToken));

				gameConfigComponent.Save();

				fmt::print(fg(fmt::color::green), "created player {}\n", nickname);

				UpdateStatus(Nz::SimpleTextDrawer::Draw(fmt::format("Player {} has been created!", nickname), 48, Nz::TextStyle_Regular, Nz::Color::Green()));
				m_nextState = m_previousState;
				m_nextStateTimer = Nz::Time::Seconds(2.f);
			});

			return true;
		});
	}

	void CreatePlayerState::UpdateStatus(const Nz::AbstractTextDrawer& textDrawer)
	{
		m_status->UpdateText(textDrawer);
		m_status->Resize(m_status->GetPreferredSize());
		m_status->Center();
		m_status->Show();

		LayoutWidgets(GetStateData().canvas->GetSize());
	}
}
