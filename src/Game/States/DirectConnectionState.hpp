// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_DIRECTCONNECTIONSTATE_HPP
#define TSOM_GAME_STATES_DIRECTCONNECTIONSTATE_HPP

#include <Game/States/WidgetState.hpp>
#include <string>

namespace Nz
{
	class BoxLayout;
	class ButtonWidget;
	class LabelWidget;
	class TextAreaWidget;
}

namespace tsom
{
	class DirectConnectionState : public WidgetState
	{
		public:
			DirectConnectionState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState);
			~DirectConnectionState() = default;

			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnConnectPressed();

			std::shared_ptr<Nz::State> m_nextState;
			std::shared_ptr<Nz::State> m_previousState;
			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_connectButton;
			Nz::TextAreaWidget* m_serverAddressArea;
			Nz::TextAreaWidget* m_loginArea;
			bool m_autoConnect;
	};
}

#include <Game/States/DirectConnectionState.inl>

#endif // TSOM_GAME_STATES_DIRECTCONNECTIONSTATE_HPP
