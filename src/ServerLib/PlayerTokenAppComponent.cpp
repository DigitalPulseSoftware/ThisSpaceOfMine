// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerConstants.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <chrono>

namespace tsom
{
	void PlayerTokenAppComponent::Register(const Nz::Uuid& uuid, std::string apiUrl, std::string refreshToken)
	{
		PlayerToken& playerToken = m_playerTokens[uuid];
		playerToken.refreshToken = std::move(refreshToken);
		playerToken.apiUrl = std::move(apiUrl);
	}

	void PlayerTokenAppComponent::QueueRequest(const Nz::Uuid& playerUuid, Nz::WebRequestMethod method, std::string route, const nlohmann::json& body, RequestCallback callback)
	{
		auto it = m_playerTokens.find(playerUuid);
		if (it == m_playerTokens.end())
		{
			fmt::print(fg(fmt::color::red), "player {} has no token entry, {} request cannot be queued\n", playerUuid.ToString(), route);
			callback(0, "no player token entry");
			return;
		}

		PlayerToken& playerToken = it.value();
		playerToken.pendingRequests.push_back({
			.jsonBody = body.dump(),
			.route = std::move(route),
			.method = method,
			.callback = std::move(callback)
		});
	}

	void PlayerTokenAppComponent::Update(Nz::Time elapsedTime)
	{
		if (m_throttleTime > Nz::Time::Zero())
		{
			m_throttleTime -= elapsedTime;
			if (m_throttleTime > Nz::Time::Zero())
				return;

			m_throttleTime = Nz::Time::Zero();
		}

		for (const Nz::Uuid& playerUuid : m_pendingRefreshTokens)
		{
			auto it = m_playerTokens.find(playerUuid);
			if (it != m_playerTokens.end())
				RefreshAccessToken(playerUuid, it.value());
		}
		m_pendingRefreshTokens.clear();

		// Send pending requests
		for (auto it = m_playerTokens.begin(); it != m_playerTokens.end(); ++it)
		{
			const Nz::Uuid& playerUuid = it.key();
			auto& playerToken = it.value();
			if (playerToken.pendingRequests.empty())
				continue;

			// Check if access is expired
			if (Nz::Timestamp::Now() >= playerToken.accessToken.expirationTime)
			{
				if (!playerToken.isRefreshing)
				{
					QueueTokenRefresh(playerUuid);
					playerToken.isRefreshing = true;
				}

				continue;
			}

			for (auto& request : playerToken.pendingRequests)
				PerformRequest(playerUuid, playerToken, std::move(request));

			playerToken.pendingRequests.clear();
		}
	}

	void PlayerTokenAppComponent::PerformRequest(const Nz::Uuid& playerUuid, const PlayerToken& playerToken, Request&& requestData)
	{
		auto& webService = GetApp().GetComponent<Nz::WebServiceAppComponent>();

		webService.QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetMethod(requestData.method);
			request.SetURL(fmt::format("{}{}", playerToken.apiUrl, requestData.route));
			request.SetHeader("Authorization", "Bearer " + playerToken.accessToken.token);

			if (requestData.method != Nz::WebRequestMethod::Get)
				request.SetJSonContent(requestData.jsonBody);

			request.SetResultCallback([this, uuid = playerUuid, requestData = std::move(requestData)](Nz::WebRequestResult&& result) mutable
			{
				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "player API request {} failed for player {}: {}\n", requestData.route, uuid.ToString(), result.GetErrorMessage());
					requestData.callback(0, result.GetErrorMessage());
					return;
				}

				Nz::UInt32 statusCode = result.GetStatusCode();
				switch (statusCode)
				{
					case 401: //< Unauthorized (token expired)
					{
						auto playerIt = m_playerTokens.find(uuid);
						if (playerIt != m_playerTokens.end())
						{
							auto& playerToken = playerIt.value();
							playerToken.pendingRequests.push_back(std::move(requestData));

							playerToken.isRefreshing = true;
							QueueTokenRefresh(uuid);
						}
						return;
					}

					case 429: //< Too many request
					{
						// TODO: Extract time to wait from header
						m_throttleTime += Nz::Time::Second();

						auto playerIt = m_playerTokens.find(uuid);
						if (playerIt != m_playerTokens.end())
						{
							auto& playerToken = playerIt.value();
							playerToken.pendingRequests.push_back(std::move(requestData));
						}
						return;
					}

					default:
						requestData.callback(statusCode, result.GetBody());
						break;
				}
			});

			return true;
		});
	}

	void PlayerTokenAppComponent::RefreshAccessToken(const Nz::Uuid& playerUuid, const PlayerToken& playerToken)
	{
		auto& webService = GetApp().GetComponent<Nz::WebServiceAppComponent>();

		webService.QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetMethod(Nz::WebRequestMethod::Post);
			request.SetURL(playerToken.apiUrl + "/v1/refresh");
			request.SetJSonContent("{}");
			request.SetHeader("Authorization", "Bearer " + playerToken.refreshToken);

			request.SetResultCallback([this, uuid = playerUuid](Nz::WebRequestResult&& result)
			{
				auto playerIt = m_playerTokens.find(uuid);
				if (playerIt != m_playerTokens.end())
					playerIt.value().isRefreshing = false;

				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to refresh token for player {}: {}\n", uuid.ToString(), result.GetErrorMessage());
					return;
				}

				if (result.GetStatusCode() != 200)
				{
					if (result.GetStatusCode() == 429)
					{
						// Too many request, try again and increase throttle time
						// TODO: Extract time to wait from header
						m_throttleTime += Nz::Time::Second();

						if (playerIt != m_playerTokens.end())
						{
							QueueTokenRefresh(uuid);
							playerIt.value().isRefreshing = true;
						}
					}
					else
						fmt::print(fg(fmt::color::red), "failed to refresh token for player {} (error {}): {}\n", uuid.ToString(), result.GetStatusCode(), result.GetBody());

					return;
				}

				try
				{
					nlohmann::json response = nlohmann::json::parse(result.GetBody());

					if (playerIt == m_playerTokens.end())
						return; //< player disconnected before we got the response

					PlayerToken& playerToken = playerIt.value();
					playerToken.accessToken.token = response["access_token"];
					playerToken.accessToken.expirationTime = Nz::Timestamp::Now() + Nz::Time::Seconds<Nz::Int64>(response["access_token_expires_in"]);
					playerToken.refreshToken = response["refresh_token"];
					playerToken.apiUrl = response.value("apiUrl", playerToken.apiUrl);
				}
				catch (const std::exception& e)
				{
					fmt::print(fg(fmt::color::red), "failed to refresh token for player {}: {}\n", uuid.ToString(), e.what());
					return;
				}
			});

			return true;
		});
	}

	void PlayerTokenAppComponent::QueueTokenRefresh(const Nz::Uuid& playerUuid)
	{
		auto it = std::find(m_pendingRefreshTokens.begin(), m_pendingRefreshTokens.end(), playerUuid);
		if (it == m_pendingRefreshTokens.end())
			m_pendingRefreshTokens.push_back(playerUuid);
	}
}
