// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_BACKGROUNDSTATE_HPP
#define TSOM_GAME_STATES_BACKGROUNDSTATE_HPP

#include <Game/States/WidgetState.hpp>

namespace Nz
{
	class ApplicationBase;
	class BaseWidget;
	class StateMachine;
	class WindowSwapchain;
}

namespace tsom
{
	class BackgroundState : public WidgetState
	{
		public:
			BackgroundState(std::shared_ptr<StateData> stateData);
			~BackgroundState() = default;

			/*void Enter(Nz::StateMachine& fsm) override;
			void Leave(Nz::StateMachine& fsm) override;*/
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			entt::handle m_camera;
			entt::handle m_skybox;
	};
}

#include <Game/States/BackgroundState.inl>

#endif // TSOM_GAME_STATES_BACKGROUNDSTATE_HPP
