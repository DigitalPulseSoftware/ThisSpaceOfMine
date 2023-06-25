// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_CONNECTIONSTATE_HPP
#define TSOM_CLIENT_STATES_CONNECTIONSTATE_HPP

#include <Game/States/StateData.hpp>
#include <Game/States/WidgetState.hpp>
#include <CommonLib/NetworkReactor.hpp>
#include <CommonLib/NetworkSession.hpp>

namespace Nz
{
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
			ConnectionState(std::shared_ptr<StateData> stateData);
			~ConnectionState() = default;

			void Connect(const Nz::IpAddress& serverAddress, std::shared_ptr<Nz::State> previousState, std::shared_ptr<Nz::State> nextState);
			void Disconnect();

			inline bool HasSession() const;

			void LayoutWidgets(const Nz::Vector2f& newSize) override;

			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			std::optional<NetworkSession> m_serverSession;
			std::shared_ptr<Nz::State> m_connectedState;
			std::shared_ptr<Nz::State> m_previousState;
			std::shared_ptr<Nz::State> m_nextState;
			Nz::LabelWidget* m_connectingLabel;
			NetworkReactor m_reactor;
			Nz::Time m_nextStateTimer;
	};
}

#include <Game/States/ConnectionState.inl>

#endif // TSOM_CLIENT_STATES_CONNECTIONSTATE_HPP
