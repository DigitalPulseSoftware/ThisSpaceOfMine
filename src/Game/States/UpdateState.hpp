// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_UPDATESTATE_HPP
#define TSOM_GAME_STATES_UPDATESTATE_HPP

#include <CommonLib/UpdaterAppComponent.hpp>
#include <Game/States/WidgetState.hpp>
#include <string>

namespace Nz
{
	class ButtonWidget;
	class BoxLayout;
	class ProgressBarWidget;
	class SimpleLabelWidget;
}

namespace tsom
{
	class UpdateState : public WidgetState
	{
		public:
			UpdateState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState, UpdateInfo updateInfo);
			~UpdateState() = default;

			void Enter(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void UpdateProgressBar(std::size_t activeDownloadCount, Nz::UInt64 downloaded, Nz::UInt64 total);

			NazaraSlot(UpdaterAppComponent, OnDownloadProgress, m_onDownloadProgressSlot);
			NazaraSlot(UpdaterAppComponent, OnUpdateFailed, m_onUpdateFailed);
			NazaraSlot(UpdaterAppComponent, OnUpdateStarting, m_onUpdateStarting);

			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_cancelButton;
			Nz::ProgressBarWidget* m_progressBar;
			Nz::SimpleLabelWidget* m_downloadLabel;
			Nz::SimpleLabelWidget* m_progressionLabel;
			std::shared_ptr<Nz::State> m_previousState;
			UpdateInfo m_updateInfo;
			bool m_isCancelled;
	};
}

#include <Game/States/UpdateState.inl>

#endif // TSOM_GAME_STATES_UPDATESTATE_HPP
