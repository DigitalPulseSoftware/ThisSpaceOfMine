// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UPDATERAPPCOMPONENT_HPP
#define TSOM_COMMONLIB_UPDATERAPPCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/DownloadManager.hpp>
#include <CommonLib/UpdateInfo.hpp>
#include <Nazara/Core/ApplicationComponent.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/Result.hpp>
#include <NazaraUtils/Signal.hpp>
#include <functional>

namespace tsom
{
	class ConfigFile;

	class TSOM_COMMONLIB_API UpdaterAppComponent final : public Nz::ApplicationComponent
	{
		public:
			UpdaterAppComponent(Nz::ApplicationBase& app, ConfigFile& configFile);
			UpdaterAppComponent(const UpdaterAppComponent&) = delete;
			UpdaterAppComponent(UpdaterAppComponent&&) = delete;
			~UpdaterAppComponent() = default;

			void CancelUpdate();

			void DownloadAndUpdate(const UpdateInfo& updateInfo, bool downloadAssets, bool downloadBinaries);

			void FetchLastVersion(std::function<void(Nz::Result<UpdateInfo, std::string>&& updateInfo)>&& callback);

			UpdaterAppComponent& operator=(const UpdaterAppComponent&) = delete;
			UpdaterAppComponent& operator=(UpdaterAppComponent&&) = delete;

			NazaraSignal(OnDownloadProgress, std::size_t /*activeDownloadCount*/, Nz::UInt64 /*totalDownloaded*/, Nz::UInt64 /*totalSize*/);
			NazaraSignal(OnUpdateFailed);
			NazaraSignal(OnUpdateStarting);

		private:
			void StartUpdaterAndQuit();
			void Update(Nz::Time elapsedTime) override;
			void UpdateProgression();

			Nz::FixedVector<std::shared_ptr<const DownloadManager::Download>, 3> m_activeDownloads;
			Nz::FixedVector<std::shared_ptr<const DownloadManager::Download>, 3> m_updateArchives;
			std::shared_ptr<const DownloadManager::Download> m_updaterDownload;
			DownloadManager m_downloadManager;
			ConfigFile& m_configFile;
	};
}

#include <CommonLib/UpdaterAppComponent.inl>

#endif // TSOM_COMMONLIB_UPDATERAPPCOMPONENT_HPP
