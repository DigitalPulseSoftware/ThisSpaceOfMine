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
	class MenuState : public WidgetState
	{
		public:
			MenuState(Nz::BaseWidget* parentWidget, Nz::EnttWorld& world, std::shared_ptr<Nz::State> gameState);
			~MenuState() = default;

			/*void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;*/
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;

			std::shared_ptr<Nz::State> m_gameState;
			std::shared_ptr<Nz::State> m_nextState;
			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_connectButton;
			Nz::TextAreaWidget* m_loginArea;
	};
}

#include <Game/States/MenuState.inl>

#endif // TSOM_CLIENT_STATES_MENUSTATE_HPP
