// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/DownloadManager.hpp>
#include <CommonLib/Utils.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/std.h>
#include <filesystem>

namespace tsom
{
	auto DownloadManager::QueueDownload(std::filesystem::path filepath, const std::string& downloadUrl, Nz::UInt64 expectedSize, std::string expectedHash, bool force) -> std::shared_ptr<const Download>
	{
		auto* webService = m_application.TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
			return {};

		if (filepath.empty())
			filepath = Nz::Utf8Path(downloadUrl).filename();

		if (!filepath.has_extension())
			filepath.replace_extension(Nz::Utf8Path(downloadUrl).extension());

		std::shared_ptr<Download> download = std::make_shared<Download>();
		download->filepath = std::move(filepath);
		download->totalSize = expectedSize;

		if (!force && !expectedHash.empty())
		{
			if (std::filesystem::is_regular_file(download->filepath) && std::filesystem::file_size(download->filepath) == expectedSize)
			{
				if (Nz::File::ComputeHash(Nz::HashType::SHA256, download->filepath).ToHex() == expectedHash)
				{
					download->isFinished = true;
					return download;
				}
			}
		}

		if (!download->file.Open(download->filepath, Nz::OpenMode::Write))
		{
			fmt::print(fg(fmt::color::red), "failed to create file {0}", download->filepath);
			return {};
		}

		webService->QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupGet();
			request.SetURL(downloadUrl);

			request.SetResultCallback([activeDownloads = m_activeDownloads, download, expectedHash](Nz::WebRequestResult&& result)
			{
				activeDownloads->erase(std::find(activeDownloads->begin(), activeDownloads->end(), download));

				download->isFinished = true;
				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to download {0}: {1}\n", download->filepath, result.GetErrorMessage());
					download->file.Delete();
					download->OnDownloadFailed(*download);
					return;
				}

				if (!expectedHash.empty())
				{
					if (std::string fileHash = download->hasher.End().ToHex(); fileHash != expectedHash)
					{
						fmt::print(fg(fmt::color::red), "failed to download {0}: hash doesn't match (file hash {1} doesn't match expected hash {2})\n", download->filepath, fileHash, expectedHash);
						download->file.Delete();
						download->OnDownloadFailed(*download);
						return;
					}
				}

				download->file.Close();
				fmt::print(fg(fmt::color::green), "{} download succeeded!\n", download->filepath);
				download->OnDownloadFinished(*download);
			});

			if (!expectedHash.empty())
			{
				download->hasher.Begin();
				request.SetDataCallback([download](const void* data, std::size_t length) mutable
				{
					if (download->file.Write(data, length) != length)
						return false;

					download->hasher.Append(static_cast<const Nz::UInt8*>(data), length);
					return true;
				});
			}
			else
			{
				request.SetDataCallback([download](const void* data, std::size_t length) mutable
				{
					if (download->file.Write(data, length) != length)
						return false;

					return true;
				});
			}

			request.SetOptions(Nz::WebRequestOption::FailOnError | Nz::WebRequestOption::FollowRedirects);

			request.SetProgressCallback([this, download](std::size_t bytesReceived, std::size_t bytesTotal)
			{
				if (download->isCancelled)
					return false;

				if (bytesTotal != 0 && download->totalSize != 0 && bytesTotal != download->totalSize)
				{
					fmt::print(fg(fmt::color::red), "error when downloading {0}: file size ({1}) doesn't match expected size ({2})!\n", download->filepath, ByteToString(bytesTotal), ByteToString(download->totalSize));
					return false;
				}

				download->downloadedSize = bytesReceived;
				download->OnDownloadProgress(*download);
				return true;
			});

			return true;
		});

		m_activeDownloads->push_back(download);
		return download;
	}
}
