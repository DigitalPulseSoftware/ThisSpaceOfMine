// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_AUTHENTICATIONSTATE_HPP
#define TSOM_GAME_STATES_AUTHENTICATIONSTATE_HPP

#include <Game/States/StateData.hpp>
#include <Game/States/WidgetState.hpp>
#include <Nazara/Core/Time.hpp>

namespace Nz
{
	class AbstractTextDrawer;
	class LabelWidget;
}

namespace tsom
{
	class AuthenticationState : public WidgetState
	{
		public:
			AuthenticationState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState, std::string_view token);
			~AuthenticationState() = default;

			void LayoutWidgets(const Nz::Vector2f& newSize) override;

		private:
			void UpdateStatus(const Nz::AbstractTextDrawer& textDrawer);

			std::shared_ptr<Nz::State> m_nextState;
			std::shared_ptr<Nz::State> m_previousState;
			Nz::LabelWidget* m_statusLabel;
			Nz::Time m_nextStateTimer;
	};
}

#include <Game/States/AuthenticationState.inl>

#endif // TSOM_GAME_STATES_AUTHENTICATIONSTATE_HPP
