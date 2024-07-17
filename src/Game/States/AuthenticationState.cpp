// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE
#if 0

#include <Game/States/AuthenticationState.hpp>
#include <Game/GameConfigAppComponent.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/LabelWidget.hpp>
#include <fmt/core.h>

namespace nlohmann
{
	template<>
	struct adl_serializer<semver::version>
	{
		static void from_json(const nlohmann::json& json, semver::version& downloadInfo)
		{
			downloadInfo.from_string(json.template get<std::string_view>());
		}
	};

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
	struct adl_serializer<tsom::UpdateInfo>
	{
		static void from_json(const nlohmann::json& json, tsom::UpdateInfo& updateInfo)
		{
			updateInfo.assetVersion = json.at("assets_version");
			updateInfo.binaryVersion = json.at("version");
			updateInfo.assets = json.at("assets");
			updateInfo.binaries = json.at("binaries");
			updateInfo.updater = json.at("updater");
		}
	};
}

namespace tsom
{
	AuthenticationState::AuthenticationState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState, std::string_view token) :
	WidgetState(std::move(stateData)),
	m_previousState(std::move(previousState))
	{
		m_statusLabel = CreateWidget<Nz::LabelWidget>();

		auto& webService = GetStateData().app->GetComponent<Nz::WebServiceAppComponent>();

		webService.QueueRequest([&](Nz::WebRequest& request)
		{
			auto& gameConfig = GetStateData().app->GetComponent<GameConfigAppComponent>().GetConfig();

			request.SetupPost();
			request.SetURL(fmt::format("{}/v1/player/authenticate", gameConfig.GetStringValue("Api.Url")));
			request.SetServiceName("TSOM Authentication");
			request.SetResultCallback([this](Nz::WebRequestResult&& result)
			{
				if (!result.HasSucceeded())
				{
					UpdateStatus(Nz::SimpleTextDrawer::Draw(fmt::format("failed to authenticate: {}", result.GetErrorMessage()), 36, Nz::TextStyle_Regular, Nz::Color::Red()));
					m_nextState = m_previousState;
					m_nextStateTimer = Nz::Time::Seconds(3);
					return;
				}

				if (result.GetStatusCode() != 200)
				{
					//TODO: parse errors
					UpdateStatus(Nz::SimpleTextDrawer::Draw(fmt::format("failed to authenticate: {}", result.GetErrorMessage()), 36, Nz::TextStyle_Regular, Nz::Color::Red()));
					m_nextState = m_previousState;
					m_nextStateTimer = Nz::Time::Seconds(3);
					return;
				}

				UpdateInfo updateInfo;
				try
				{
					updateInfo = nlohmann::json::parse(result.GetBody());
				}
				catch (const std::exception& e)
				{
					cb(Nz::Err(fmt::format("failed to parse version data: {0}", e.what())));
					return;
				}

			});

			return true;
		});
	}

	void AuthenticationState::LayoutWidgets(const Nz::Vector2f& newSize)
	{
		m_statusLabel->SetPosition(newSize * Nz::Vector2f(0.5f, 0.9f) - m_statusLabel->GetSize() * 0.5f);
	}

	void AuthenticationState::UpdateStatus(const Nz::AbstractTextDrawer& textDrawer)
	{
		m_statusLabel->UpdateText(textDrawer);
		m_statusLabel->Resize(m_statusLabel->GetPreferredSize());
		m_statusLabel->Center();
		m_statusLabel->Show();

		LayoutWidgets(GetStateData().canvas->GetSize());
	}
}

#endif
