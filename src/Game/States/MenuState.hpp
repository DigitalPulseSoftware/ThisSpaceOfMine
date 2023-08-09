// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_MENUSTATE_HPP
#define TSOM_CLIENT_STATES_MENUSTATE_HPP

#include <Game/States/WidgetState.hpp>

namespace Nz
{
	class BoxLayout;
	class ButtonWidget;
	class TextAreaWidget;
}

namespace tsom
{
	class ConnectionState;

	class MenuState : public WidgetState
	{
		public:
			MenuState(std::shared_ptr<StateData> stateData, std::shared_ptr<ConnectionState> connectionState);
			~MenuState() = default;

			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnConnectPressed();

			std::weak_ptr<ConnectionState> m_connectionState;
			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_connectButton;
			Nz::TextAreaWidget* m_serverAddressArea;
			Nz::TextAreaWidget* m_loginArea;
			bool m_autoConnect;
	};
}

#include <Game/States/MenuState.inl>

#endif // TSOM_CLIENT_STATES_MENUSTATE_HPP
