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
	class ImageWidget;
	class LabelWidget;
	class SimpleLabelWidget;
}

namespace tsom
{
	class ConnectionState;

	class MenuState : public WidgetState
	{
		public:
			MenuState(std::shared_ptr<StateData> stateData);
			~MenuState() = default;

			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;

			std::optional<UpdateInfo> m_newVersionInfo;
			std::shared_ptr<Nz::State> m_nextState;
			Nz::Time m_accumulator;
			Nz::BoxLayout* m_buttonLayout;
			Nz::ImageWidget* m_logo;
			Nz::SimpleLabelWidget* m_title;
			Nz::SimpleLabelWidget* m_titleBackground;
			Nz::ButtonWidget* m_playButton;
			Nz::ButtonWidget* m_quitGameButton;
			bool m_autoConnect;
			float m_logoBasePositionY;
	};
}

#include <Game/States/MenuState.inl>

#endif // TSOM_GAME_STATES_MENUSTATE_HPP
