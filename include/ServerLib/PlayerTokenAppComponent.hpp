// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_PLAYERTOKENAPPCOMPONENT_HPP
#define TSOM_SERVERLIB_PLAYERTOKENAPPCOMPONENT_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <Nazara/Core/Timestamp.hpp>
#include <Nazara/Core/Uuid.hpp>
#include <Nazara/Network/WebRequest.hpp>
#include <nlohmann/json_fwd.hpp>
#include <tsl/hopscotch_map.h>
#include <string>
#include <vector>

namespace tsom
{
	class TSOM_SERVERLIB_API PlayerTokenAppComponent : public Nz::ApplicationComponent
	{
		public:
			using RequestCallback = std::function<void(Nz::UInt32 code, const std::string& body)>;

			inline PlayerTokenAppComponent(Nz::ApplicationBase& app);
			PlayerTokenAppComponent(const PlayerTokenAppComponent&) = delete;
			PlayerTokenAppComponent(PlayerTokenAppComponent&&) = delete;
			~PlayerTokenAppComponent() = default;

			void Register(const Nz::Uuid& playerUuid, std::string apiUrl, std::string refreshToken);

			void QueueRequest(const Nz::Uuid& playerUuid, Nz::WebRequestMethod method, std::string route, const nlohmann::json& body, RequestCallback callback);

			void Update(Nz::Time elapsedTime) override;

			PlayerTokenAppComponent& operator=(const PlayerTokenAppComponent&) = delete;
			PlayerTokenAppComponent& operator=(PlayerTokenAppComponent&&) = delete;

		private:
			struct PlayerToken;
			struct Request;

			void PerformRequest(const Nz::Uuid& playerUuid, const PlayerToken& playerToken, Request&& requestData);
			void RefreshAccessToken(const Nz::Uuid& playerUuid, const PlayerToken& playerToken);
			void QueueTokenRefresh(const Nz::Uuid& playerUuid);

			struct AccessToken
			{
				std::string token;
				Nz::Timestamp expirationTime;
			};

			struct Request
			{
				std::string jsonBody;
				std::string route;
				Nz::WebRequestMethod method;
				RequestCallback callback;
			};

			struct PlayerToken
			{
				AccessToken accessToken;
				std::string refreshToken;
				std::string apiUrl;
				std::vector<Request> pendingRequests;
				bool isRefreshing = false;
			};

			std::vector<Nz::Uuid> m_pendingRefreshTokens;
			tsl::hopscotch_map<Nz::Uuid, PlayerToken> m_playerTokens;
			Nz::Time m_throttleTime; //< TODO: Handle throttle per API url
	};
}

#include <ServerLib/PlayerTokenAppComponent.inl>

#endif // TSOM_SERVERLIB_PLAYERTOKENAPPCOMPONENT_HPP
