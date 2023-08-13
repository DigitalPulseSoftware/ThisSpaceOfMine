// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_DEBUGINFOSTATE_HPP
#define TSOM_CLIENT_STATES_DEBUGINFOSTATE_HPP

#include <Game/States/WidgetState.hpp>
#include <Nazara/Utility/RichTextDrawer.hpp>

namespace Nz
{
	class LabelWidget;
}

namespace tsom
{
	class DebugInfoState : public WidgetState
	{
		public:
			DebugInfoState(std::shared_ptr<StateData> stateData);
			~DebugInfoState() = default;

			void Enter(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;

			Nz::LabelWidget* m_fpsEntity;
			Nz::MillisecondClock m_updateClock;
			Nz::RichTextDrawer m_textDrawer;
			unsigned int m_fpsCounter;
	};
}

#include <Game/States/DebugInfoState.inl>

#endif // TSOM_CLIENT_STATES_DEBUGINFOSTATE_HPP
