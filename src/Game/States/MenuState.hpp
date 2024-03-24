// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_MENUSTATE_HPP
#define TSOM_GAME_STATES_MENUSTATE_HPP

#include <CommonLib/UpdateInfo.hpp>
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
	class ConnectionState;

	class MenuState : public WidgetState
	{
		public:
			MenuState(std::shared_ptr<StateData> stateData);
			~MenuState() = default;

			void Enter(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void CheckVersion();
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnConnectPressed();
			void OnUpdateInfoReceived(UpdateInfo&& updateInfo);
			void OnUpdatePressed();

			std::optional<UpdateInfo> m_newVersionInfo;
			std::shared_ptr<Nz::State> m_nextState;
			Nz::BoxLayout* m_layout;
			Nz::BoxLayout* m_updateLayout;
			Nz::ButtonWidget* m_connectButton;
			Nz::ButtonWidget* m_updateButton;
			Nz::LabelWidget* m_updateLabel;
			Nz::TextAreaWidget* m_serverAddressArea;
			Nz::TextAreaWidget* m_loginArea;
			bool m_autoConnect;
	};
}

#include <Game/States/MenuState.inl>

#endif // TSOM_GAME_STATES_MENUSTATE_HPP
