// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_UPDATESTATE_HPP
#define TSOM_CLIENT_STATES_UPDATESTATE_HPP

#include <Game/States/WidgetState.hpp>
#include <Game/States/UpdateInfo.hpp>
#include <Nazara/Network/WebService.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <string>

namespace Nz
{
	class ButtonWidget;
	class BoxLayout;
	class LabelWidget;
	class ProgressBarWidget;
}

namespace tsom
{
	class UpdateState : public WidgetState
	{
		public:
			UpdateState(std::shared_ptr<StateData> stateData, std::shared_ptr<Nz::State> previousState, std::shared_ptr<Nz::WebService> webService, UpdateInfo updateInfo);
			~UpdateState() = default;

			void Enter(Nz::StateMachine& fsm) override;
			bool Update(Nz::StateMachine& fsm, Nz::Time elapsedTime) override;

		private:
			struct PendingDownload;

			void LayoutWidgets(const Nz::Vector2f& newSize) override;
			void StartDownload(PendingDownload& pendingDownload);
			void StartUpdate();
			void UpdateProgressBar();

			struct PendingDownload
			{
				std::string_view name;
				const UpdateInfo::DownloadInfo* info;
				Nz::UInt64 downloadedSize;
				std::string filename;
				bool isFinished = false;
			};

			Nz::BoxLayout* m_layout;
			Nz::ButtonWidget* m_cancelButton;
			Nz::FixedVector<PendingDownload, 3> m_pendingDownloads;
			Nz::LabelWidget* m_downloadLabel;
			Nz::LabelWidget* m_progressionLabel;
			Nz::ProgressBarWidget* m_progressBar;
			std::shared_ptr<Nz::State> m_previousState;
			std::shared_ptr<Nz::WebService> m_webService;
			UpdateInfo m_updateInfo;
			bool m_isCancelled;
			bool m_hasUpdateStarted;
	};
}

#include <Game/States/UpdateState.inl>

#endif // TSOM_CLIENT_STATES_UUPDATESTATE_HPP
