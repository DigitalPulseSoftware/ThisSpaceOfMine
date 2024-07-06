// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_VERSIONCHECKSTATE_HPP
#define TSOM_GAME_STATES_VERSIONCHECKSTATE_HPP

#include <CommonLib/UpdateInfo.hpp>
#include <Game/States/WidgetState.hpp>
#include <string>

namespace Nz
{
	class BoxLayout;
	class ButtonWidget;
	class LabelWidget;
}

namespace tsom
{
	class ConnectionState;

	class VersionCheckState : public WidgetState
	{
		public:
			VersionCheckState(std::shared_ptr<StateData> stateData);
			~VersionCheckState() = default;

			void Enter(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void CheckVersion();
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void OnUpdateInfoReceived(UpdateInfo&& updateInfo);
			void OnUpdatePressed();

			std::optional<UpdateInfo> m_newVersionInfo;
			std::shared_ptr<Nz::State> m_nextState;
			Nz::BoxLayout* m_updateLayout;
			Nz::ButtonWidget* m_updateButton;
			Nz::LabelWidget* m_updateLabel;
	};
}

#include <Game/States/VersionCheckState.inl>

#endif // TSOM_GAME_STATES_VERSIONCHECKSTATE_HPP
