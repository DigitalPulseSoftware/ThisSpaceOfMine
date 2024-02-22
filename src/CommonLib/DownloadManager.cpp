// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/DownloadManager.hpp>
#include <CommonLib/Utils.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/std.h>
#include <filesystem>

namespace tsom
{
	auto DownloadManager::QueueDownload(std::string name, const std::filesystem::path& filepath, const std::string& downloadUrl, Nz::UInt64 expectedSize, std::string expectedHash, bool force) -> std::shared_ptr<const Download>
	{
		auto* webService = m_application.TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
			return {};

		std::shared_ptr<Download> download = std::make_shared<Download>();
		download->name = std::move(name);
		download->totalSize = expectedSize;

		if (!force && !expectedHash.empty())
		{
			if (std::filesystem::is_regular_file(filepath) && std::filesystem::file_size(filepath) == expectedSize)
			{
				if (Nz::File::ComputeHash(Nz::HashType::SHA256, filepath).ToHex() == expectedHash)
				{
					download->isFinished = true;
					return download;
				}
			}
		}

		if (!download->file.Open(filepath, Nz::OpenMode::Write))
		{
			fmt::print(fg(fmt::color::red), "failed to create file {0}", filepath);
			return {};
		}

		webService->QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupGet();
			request.SetURL(downloadUrl);

			request.SetResultCallback([this, download, expectedHash](Nz::WebRequestResult&& result)
			{
				download->isFinished = true;

				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to download {0}: {1}\n", download->name, result.GetErrorMessage());
					download->file.Delete();
					download->OnDownloadFailed(*download);
					return;
				}

				if (!expectedHash.empty())
				{
					if (std::string fileHash = download->hasher.End().ToHex(); fileHash != expectedHash)
					{
						fmt::print(fg(fmt::color::red), "failed to download {0}: hash doesn't match (file hash {1} doesn't match expected hash {2})\n", download->name, fileHash, expectedHash);
						download->file.Delete();
						download->OnDownloadFailed(*download);
						return;
					}
				}

				fmt::print(fg(fmt::color::green), "{} download succeeded!\n", download->name);
				download->OnDownloadFinished(*download);
			});

			if (!expectedHash.empty())
			{
				download->hasher.Begin();
				request.SetDataCallback([this, download](const void* data, std::size_t length) mutable
				{
					if (download->file.Write(data, length) != length)
						return false;

					download->hasher.Append(static_cast<const Nz::UInt8*>(data), length);
					return true;
				});
			}
			else
			{
				request.SetDataCallback([this, download](const void* data, std::size_t length) mutable
				{
					if (download->file.Write(data, length) != length)
						return false;

					return true;
				});
			}

			request.SetOptions(Nz::WebRequestOption::FailOnError | Nz::WebRequestOption::FollowRedirects);

			request.SetProgressCallback([this, download](std::size_t bytesReceived, std::size_t bytesTotal)
			{
				if (m_isCancelled)
					return false;

				if (bytesTotal != 0 && download->totalSize != 0 && bytesTotal != download->totalSize)
				{
					fmt::print(fg(fmt::color::red), "error when downloading {0}: file size ({1}) doesn't match expected size ({2})!\n", download->name, FormatSize(bytesTotal), FormatSize(download->totalSize));
					return false;
				}

				download->downloadedSize = bytesReceived;
				download->OnDownloadProgress(*download);
				return true;
			});

			return true;
		});

		m_pendingDownloads.push_back(download);

		return download;
	}
}
