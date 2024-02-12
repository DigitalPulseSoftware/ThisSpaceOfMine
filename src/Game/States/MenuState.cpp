// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Game/States/MenuState.hpp>
#include <Game/States/ConnectionState.hpp>
#include <Game/States/GameState.hpp>
#include <Game/States/UpdateState.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/Version.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/StateMachine.hpp>
#include <Nazara/Core/StringExt.hpp>
#include <Nazara/Network/Algorithm.hpp>
#include <Nazara/Network/IpAddress.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <semver.hpp>
#include <optional>

namespace nlohmann
{
	template<>
	struct adl_serializer<tsom::UpdateInfo::DownloadInfo>
	{
		static void from_json(const nlohmann::json& json, tsom::UpdateInfo::DownloadInfo& downloadInfo)
		{
			downloadInfo.downloadUrl = json.at("download_url");
			downloadInfo.size = json.at("size");
			downloadInfo.sha256 = json.value("sha256", "");
		}
	};

	template<>
	struct adl_serializer<semver::version>
	{
		static void from_json(const nlohmann::json& json, semver::version& downloadInfo)
		{
			downloadInfo.from_string(json.template get<std::string_view>());
		}
	};
}

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
		auto* webService = GetStateData().app->TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
			return;

		webService->QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupGet();
			request.SetURL(fmt::format("http://tsom-api.digitalpulse.software/game_version?platform={}", BuildConfig));
			request.SetServiceName("TSOM Version Check");
			request.SetResultCallback([this](Nz::WebRequestResult&& result)
			{
				if (!result.HasSucceeded() || result.GetStatusCode() != 200)
				{
					fmt::print(fg(fmt::color::red), "failed to get version update\n");
					return;
				}

				semver::version newAssetVersion;
				semver::version newGameVersion;
				UpdateInfo::DownloadInfo assetInfo;
				UpdateInfo::DownloadInfo gameBinariesInfo;
				UpdateInfo::DownloadInfo updaterInfo;

				try
				{
					nlohmann::json json = nlohmann::json::parse(result.GetBody());

					newAssetVersion = json.at("assets_version");
					newGameVersion = json.at("version");
					assetInfo = json.at("assets");
					gameBinariesInfo = json.at("binaries");
					updaterInfo = json.at("updater");
				}
				catch (const std::exception& e)
				{
					fmt::print(fg(fmt::color::red), "failed to parse version data: {}\n", e.what());
					return;
				}

				semver::version currentGameVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);

				if (newGameVersion > currentGameVersion)
				{
					std::optional<UpdateInfo::DownloadInfo> assets;
					if (newAssetVersion > currentGameVersion)
						assets = std::move(assetInfo);

					m_newVersionInfo.emplace(UpdateInfo{
						.assets = std::move(assets),
						.version = newGameVersion.to_string(),
						.binaries = std::move(gameBinariesInfo),
						.updater = std::move(updaterInfo)
					});

					fmt::print(fg(fmt::color::yellow), "new version available: {}\n", m_newVersionInfo->version);
					m_updateButton->UpdateText(Nz::SimpleTextDrawer::Draw("Update game to " + m_newVersionInfo->version, 18, Nz::TextStyle_Regular, Nz::Color::sRGBToLinear(Nz::Color(0.13f))));
					m_updateButton->SetMaximumWidth(m_updateButton->GetPreferredWidth());

					m_updateLayout->Show();
				}
				else
					fmt::print("no new version available\n");
			});

			return true;
		});

		m_updateLayout->Hide();
		m_newVersionInfo.reset();
	}

	void MenuState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_layout->Resize({ newSize.x * 0.2f, m_layout->GetPreferredHeight() });
		m_layout->CenterHorizontal();
		m_layout->SetPosition(m_layout->GetPosition().x, newSize.y * 0.2f - m_layout->GetSize().y / 2.f);

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
			connectionState->Connect(serverAddress, login, shared_from_this());
	}

	void MenuState::OnUpdatePressed()
	{
		if (!m_newVersionInfo)
			return;

		m_nextState = std::make_shared<UpdateState>(GetStateDataPtr(), shared_from_this(), std::move(*m_newVersionInfo));
		m_newVersionInfo.reset();
	}
}
