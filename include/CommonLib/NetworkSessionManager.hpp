// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORKSESSIONMANAGER_HPP
#define TSOM_COMMONLIB_NETWORKSESSIONMANAGER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace tsom
{
	class TSOM_COMMONLIB_API NetworkSessionManager
	{
		public:
			inline NetworkSessionManager(Nz::UInt16 port, Nz::NetProtocol protocol = Nz::NetProtocol::Any, std::size_t maxSessions = MaxSessionPerManager);
			NetworkSessionManager(const NetworkSessionManager&) = delete;
			NetworkSessionManager(NetworkSessionManager&&) = delete;
			~NetworkSessionManager() = default;

			void Poll();

			inline void SendData(std::size_t peerId, Nz::UInt8 channelId, Nz::ENetPacketFlags flags, Nz::NetPacket&& packet);

			template<typename T, typename... Args> void SetDefaultHandler(Args&&... args);

			NetworkSessionManager& operator=(const NetworkSessionManager&) = delete;
			NetworkSessionManager& operator=(NetworkSessionManager&&) = delete;

			static constexpr std::size_t MaxSessionPerManager = 4095;

		private:
			using HandlerFactory = std::function<std::unique_ptr<SessionHandler>(NetworkSession* session)>;

			std::vector<std::optional<NetworkSession>> m_sessions; //< TODO: Nz::SparseVector
			HandlerFactory m_handlerFactory;
			NetworkReactor m_reactor;
	};
}

#include <CommonLib/NetworkSessionManager.inl>

#endif
