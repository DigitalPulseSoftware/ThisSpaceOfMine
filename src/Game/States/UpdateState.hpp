// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_GAME_STATES_UPDATESTATE_HPP
#define TSOM_GAME_STATES_UPDATESTATE_HPP

#include <CommonLib/DownloadManager.hpp>
#include <Game/States/UpdateInfo.hpp>
#include <Game/States/WidgetState.hpp>
#include <NazaraUtils/FixedVector.hpp>
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
			void StartUpdate();
			void UpdateProgressBar();

			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_cancelButton;
			Nz::FixedVector<std::shared_ptr<const DownloadManager::Download>, 3> m_activeDownloads;
			Nz::FixedVector<std::shared_ptr<const DownloadManager::Download>, 3> m_updateArchives;
			Nz::ProgressBarWidget* m_progressBar;
			Nz::SimpleLabelWidget* m_downloadLabel;
			Nz::SimpleLabelWidget* m_progressionLabel;
			std::shared_ptr<Nz::State> m_previousState;
			std::shared_ptr<const DownloadManager::Download> m_updaterDownload;
			DownloadManager m_downloadManager;
			UpdateInfo m_updateInfo;
			bool m_isCancelled;
			bool m_hasUpdateStarted;
	};
}

#include <Game/States/UpdateState.inl>

#endif // TSOM_GAME_STATES_UPDATESTATE_HPP
