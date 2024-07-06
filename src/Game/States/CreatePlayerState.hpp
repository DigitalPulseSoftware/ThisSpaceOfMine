// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_CREATEPLAYERSTATE_HPP
#define TSOM_GAME_STATES_CREATEPLAYERSTATE_HPP

#include <CommonLib/UpdateInfo.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/Time.hpp>
#include <string>

namespace Nz
{
	class AbstractTextDrawer;
	class BoxLayout;
	class LabelWidget;
}

namespace tsom
{
	class ConnectionState;

	class CreatePlayerState : public WidgetState
	{
		public:
			CreatePlayerState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState);
			~CreatePlayerState() = default;

			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnCreatePressed(const std::string& nickname);
			void UpdateStatus(const Nz::AbstractTextDrawer& textDrawer);

			std::shared_ptr<Nz::State> m_previousState;
			std::shared_ptr<Nz::State> m_nextState;
			Nz::BoxLayout* m_layout;
			Nz::LabelWidget* m_status;
			Nz::Time m_nextStateTimer;
	};
}

#include <Game/States/CreatePlayerState.inl>

#endif // TSOM_GAME_STATES_CREATEPLAYERSTATE_HPP
