// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/ConfigFile.hpp>
#include <CommonLib/Version.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/Process.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace nlohmann
{
	template<>
	struct adl_serializer<semver::version>
	{
		static void from_json(const nlohmann::json& json, semver::version& downloadInfo)
		{
			downloadInfo.from_string(json.template get<std::string_view>());
		}
	};

	template<>
	struct adl_serializer<tsom::UpdateInfo::DownloadInfo>
	{
		static void from_json(const nlohmann::json& json, tsom::UpdateInfo::DownloadInfo& downloadInfo)
		{
			downloadInfo.downloadUrl = json.at("download_url");
			downloadInfo.size = json.at("size");
			downloadInfo.sha256 = json.value("sha256", "");
		}
	};

	template<>
	struct adl_serializer<tsom::UpdateInfo>
	{
		static void from_json(const nlohmann::json& json, tsom::UpdateInfo& updateInfo)
		{
			updateInfo.assetVersion = json.at("assets_version");
			updateInfo.binaryVersion = json.at("version");
			updateInfo.assets = json.at("assets");
			updateInfo.binaries = json.at("binaries");
			updateInfo.updater = json.at("updater");
		}
	};
}

namespace tsom
{
	UpdaterAppComponent::UpdaterAppComponent(Nz::ApplicationBase& app, ConfigFile& configFile) :
	ApplicationComponent(app),
	m_downloadManager(app),
	m_configFile(configFile)
	{
	}

	void UpdaterAppComponent::CancelUpdate()
	{
		m_downloadManager.Cancel();
	}

	void UpdaterAppComponent::DownloadAndUpdate(const UpdateInfo& updateInfo, bool downloadAssets, bool downloadBinaries, bool downloadUpdater, bool startUpdateWhenReady)
	{
		assert(downloadAssets || downloadBinaries);
		assert(!startUpdateWhenReady || downloadUpdater);

		// just in case
		m_activeDownloads.clear();
		m_updaterDownload.reset();

		auto QueueDownload = [&](std::string_view filename, const UpdateInfo::DownloadInfo& info, bool isExecutable = false)
		{
			auto download = m_downloadManager.QueueDownload(Nz::Utf8Path(filename), info.downloadUrl, info.size, info.sha256, false, isExecutable);
			m_activeDownloads.push_back(download);

			return download;
		};

		if (downloadAssets)
			m_updateArchives.push_back(QueueDownload("autoupdate_assets", updateInfo.assets));

		if (downloadBinaries)
			m_updateArchives.push_back(QueueDownload("autoupdate_binaries", updateInfo.binaries));

		if (downloadUpdater)
			m_updaterDownload = QueueDownload("this_updater_of_mine", updateInfo.updater, true);

		for (auto& downloadPtr : m_activeDownloads)
		{
			downloadPtr->OnDownloadFailed.Connect([this](const DownloadManager::Download&)
			{
				OnUpdateFailed();
			});

			downloadPtr->OnDownloadProgress.Connect([this](const DownloadManager::Download&)
			{
				UpdateProgression();
			});

			downloadPtr->OnDownloadFinished.Connect([this, startUpdateWhenReady](const DownloadManager::Download&)
			{
				if (!m_downloadManager.HasDownloadInProgress())
				{
					OnUpdateReady();
					if (startUpdateWhenReady)
						StartUpdaterAndQuit();
				}
			});
		}

		// Can happen if all files were already downloaded
		if (!m_downloadManager.HasDownloadInProgress())
		{
			OnUpdateReady();
			if (startUpdateWhenReady)
				StartUpdaterAndQuit();
		}
	}

	void UpdaterAppComponent::FetchLastVersion(bool server, std::function<void(Nz::Result<UpdateInfo, std::string>&& updateInfo)>&& callback)
	{
		auto* webService = GetApp().TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
		{
			callback(Nz::Err("webservices are not available"));
			return;
		}

		webService->QueueRequest([&, server](Nz::WebRequest& request)
		{
			request.SetMethod(Nz::WebRequestMethod::Get);
			request.SetURL(fmt::format("{}/game_version?platform={}{}_{}", m_configFile.GetStringValue("Api.Url"), BuildPlatform, server ? "-server" : "", BuildArch));
			request.SetServiceName("TSOM Version Check");
			request.SetResultCallback([cb = std::move(callback)](Nz::WebRequestResult&& result)
			{
				if (!result.HasSucceeded())
				{
					cb(Nz::Err(fmt::format("request failed: {0}", result.GetErrorMessage())));
					return;
				}

				if (result.GetStatusCode() != 200)
				{
					cb(Nz::Err(fmt::format("request failed with code {0}: {1}", result.GetStatusCode(), result.GetBody())));
					return;
				}

				UpdateInfo updateInfo;
				try
				{
					updateInfo = nlohmann::json::parse(result.GetBody());
				}
				catch (const std::exception& e)
				{
					cb(Nz::Err(fmt::format("failed to parse version data: {0}", e.what())));
					return;
				}

				cb(std::move(updateInfo));
			});

			return true;
		});
	}

	void UpdaterAppComponent::StartUpdaterAndQuit()
	{
		OnUpdateStarting();

		if (m_updaterDownload)
		{
			Nz::Pid pid = Nz::Process::GetCurrentPid();

			std::vector<std::string> args;
			args.push_back(fmt::format("--pid={}", pid)); // pid to wait before starting update

			for (auto& downloadPtr : m_updateArchives)
			{
				args.push_back("--archive");
				args.push_back(Nz::PathToString(downloadPtr->filepath));
			}

			std::span<const char*> applicationArgs = GetApp().GetArgs();

			args.push_back("--executable");
			args.push_back(applicationArgs[0]);

			for (std::size_t i = 1; i < applicationArgs.size(); ++i)
			{
				args.push_back("--arg");
				args.push_back(applicationArgs[i]);
			}

			Nz::Result updater = Nz::Process::SpawnDetached(m_updaterDownload->filepath, args);
			if (!updater)
			{
				fmt::print(fg(fmt::color::red), "failed to start autoupdater process: {0}\n", updater.GetError());
				return;
			}
		}

		GetApp().Quit();
	}

	void UpdaterAppComponent::UpdateProgression()
	{
		std::size_t activeDownloadCount = 0;
		Nz::UInt64 totalDownloaded = 0;
		Nz::UInt64 totalSize = 0;

		for (auto& downloadPtr : m_activeDownloads)
		{
			totalDownloaded += downloadPtr->downloadedSize;
			totalSize += downloadPtr->totalSize;

			if (downloadPtr->downloadedSize != downloadPtr->totalSize)
				activeDownloadCount++;
		}

		OnDownloadProgress(activeDownloadCount, totalDownloaded, totalSize);
	}
}
