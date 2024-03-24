// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_CONNECTIONSTATE_HPP
#define TSOM_GAME_STATES_CONNECTIONSTATE_HPP

#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/NetworkSession.hpp>
#include <CommonLib/Utility/AverageValues.hpp>
#include <Game/States/StateData.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/Clock.hpp>
#include <NazaraUtils/FixedVector.hpp>

namespace Nz
{
	class AbstractTextDrawer;
	class ApplicationBase;
	class BaseWidget;
	class LabelWidget;
	class StateMachine;
	class WindowSwapchain;
}

namespace tsom
{
	class ConnectionState : public WidgetState
	{
		public:
			struct ConnectionInfo;

			ConnectionState(std::shared_ptr<StateData> stateData);
			~ConnectionState() = default;

			void Connect(const Nz::IpAddress& serverAddress, std::string nickname, std::shared_ptr<Nz::State> previousState);
			void Disconnect();

			inline const ConnectionInfo* GetConnectionInfo() const;

			inline bool HasSession() const;

			void LayoutWidgets(const Nz::Vector2f& newSize) override;

			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

			struct ConnectionInfo
			{
				ConnectionInfo() :
				downloadSpeed(20),
				uploadSpeed(20)
				{
				}

				NetworkReactor::PeerInfo peerInfo;
				AverageValues<double> downloadSpeed;
				AverageValues<double> uploadSpeed;
			};

		private:
			void PollSessionInfo();
			void UpdateStatus(const Nz::AbstractTextDrawer& textDrawer);

			std::optional<ConnectionInfo> m_connectionInfo;
			std::optional<NetworkSession> m_serverSession;
			std::shared_ptr<Nz::State> m_connectedState;
			std::shared_ptr<Nz::State> m_previousState;
			std::shared_ptr<Nz::State> m_nextState;
			std::string m_nickname;
			Nz::HighPrecisionClock m_sessionInfoClock;
			Nz::FixedVector<NetworkReactor, 2> m_reactors;
			Nz::LabelWidget* m_connectingLabel;
			Nz::Time m_nextPollTimer;
			Nz::Time m_nextStateTimer;
	};
}

#include <Game/States/ConnectionState.inl>

#endif // TSOM_GAME_STATES_CONNECTIONSTATE_HPP
