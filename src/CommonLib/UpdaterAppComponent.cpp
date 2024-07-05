// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/UpdaterAppComponent.hpp>
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
	UpdaterAppComponent::UpdaterAppComponent(Nz::ApplicationBase& app) :
	ApplicationComponent(app),
	m_downloadManager(app)
	{
	}

	void UpdaterAppComponent::CancelUpdate()
	{
		m_downloadManager.Cancel();
	}

	void UpdaterAppComponent::DownloadAndUpdate(const UpdateInfo& updateInfo, bool downloadAssets, bool downloadBinaries)
	{
		assert(downloadAssets || downloadBinaries);

		m_activeDownloads.clear(); //< just in case
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

			downloadPtr->OnDownloadFinished.Connect([this](const DownloadManager::Download&)
			{
				if (!m_downloadManager.HasDownloadInProgress())
					StartUpdaterAndQuit();
			});
		}

		// Can happen if all files were already downloaded
		if (!m_downloadManager.HasDownloadInProgress())
			StartUpdaterAndQuit();
	}

	void UpdaterAppComponent::FetchLastVersion(std::function<void(Nz::Result<UpdateInfo, std::string>&& updateInfo)>&& callback)
	{
		auto* webService = GetApp().TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
		{
			callback(Nz::Err("webservices are not available"));
			return;
		}

		webService->QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupGet();
			request.SetURL(fmt::format("http://tsom-api.digitalpulse.software/game_version?platform={}", BuildConfig));
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
					cb(Nz::Err(fmt::format("request failed with code {0}: {1}", result.GetStatusCode(), result.GetErrorMessage())));
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

		Nz::Pid pid = Nz::Process::GetCurrentPid();

		std::vector<std::string> args;
		args.push_back(std::to_string(pid)); // pid to wait before starting update

		// TODO: Add a way to retrieve executable name (or use argv[0]?)
#ifdef NAZARA_PLATFORM_WINDOWS
		args.push_back("ThisSpaceOfMine.exe");
#else
		args.push_back("./ThisSpaceOfMine");
#endif

		for (auto& downloadPtr : m_updateArchives)
			args.push_back(Nz::PathToString(downloadPtr->filepath));

		Nz::Result updater = Nz::Process::SpawnDetached(m_updaterDownload->filepath, args);
		if (!updater)
		{
			fmt::print(fg(fmt::color::red), "failed to start autoupdater process: {0}\n", updater.GetError());
			return;
		}

		GetApp().Quit();
	}

	void UpdaterAppComponent::Update(Nz::Time elapsedTime)
	{
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
