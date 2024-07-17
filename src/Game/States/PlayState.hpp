// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_PLAYSTATE_HPP
#define TSOM_GAME_STATES_PLAYSTATE_HPP

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

	class PlayState : public WidgetState
	{
		public:
			PlayState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState);
			~PlayState() = default;

			void Enter(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void FetchPlayerInfo();
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnCreateOrConnectPressed();

			std::optional<UpdateInfo> m_newVersionInfo;
			std::shared_ptr<Nz::State> m_nextState;
			std::shared_ptr<Nz::State> m_previousState;
			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_createOrConnectButton;
			Nz::ButtonWidget* m_directConnect;
			bool m_autoConnect;
	};
}

#include <Game/States/PlayState.inl>

#endif // TSOM_GAME_STATES_PLAYSTATE_HPP
