// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/DownloadManager.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <filesystem>

namespace tsom
{
	namespace
	{
		std::string FormatSize(Nz::UInt64 sizeInBytes)
		{
			constexpr std::array<std::string_view, 7> s_units = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };

			std::size_t unitIndex = 0;
			for (; unitIndex < s_units.size(); ++unitIndex)
			{
				if (sizeInBytes < 1024 * 1024)
					break;

				sizeInBytes /= 1024;
			}

			double size = 0.0;
			if (sizeInBytes > 1024 && unitIndex < s_units.size() - 1)
			{
				size = sizeInBytes / 1024.0;
				unitIndex++;
			}
			else
				size = sizeInBytes;

			return fmt::format("{:.2f} {}", size, s_units[unitIndex]);
		}
	}

	bool DownloadManager::QueueDownload(std::string name, std::filesystem::path filepath, const std::string& downloadUrl, Nz::UInt64 expectedSize, std::string expectedHash, bool force)
	{
		auto* webService = m_application.TryGetComponent<Nz::WebServiceAppComponent>();
		if (!webService)
			return false;

		std::shared_ptr<Download> download = std::make_shared<Download>();
		download->filename = std::move(name);

		m_pendingDownloads.push_back(download);

		if (!force && !expectedHash.empty())
		{
			if (std::filesystem::is_regular_file(filepath) && std::filesystem::file_size(filepath) == expectedSize)
			{
				if (Nz::File::ComputeHash(Nz::HashType::SHA256, filepath).ToHex() == expectedHash)
				{
					download->isFinished = true;
					return true;
				}
			}
		}

		if (!download->file.Open(filepath, Nz::OpenMode::Write))
			return false;

		webService->QueueRequest([&](Nz::WebRequest& request)
		{
			request.SetupGet();
			request.SetURL(downloadUrl);

			request.SetResultCallback([this, download, expectedHash](Nz::WebRequestResult&& result)
			{
				download->isFinished = true;

				if (!result.HasSucceeded())
				{
					fmt::print(fg(fmt::color::red), "failed to download {0}: {1}\n", download->filename, result.GetErrorMessage());
					download->file.Delete();
					OnDownloadFailed(this, download->filename);
					return;
				}

				if (!expectedHash.empty())
				{
					if (std::string fileHash = download->hasher.End().ToHex(); fileHash != expectedHash)
					{
						fmt::print(fg(fmt::color::red), "failed to download {0}: hash doesn't match (file hash {1} doesn't match expected hash {2})\n", download->filename, fileHash, expectedHash);
						download->file.Delete();
						OnDownloadFailed(this, download->filename);
						return;
					}
				}

				fmt::print(fg(fmt::color::green), "{} download succeeded!\n", download->filename);
				OnDownloadFinished(this, download->filename);
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

			request.SetProgressCallback([this, download, expectedSize](std::size_t bytesReceived, std::size_t bytesTotal)
			{
				if (m_isCancelled)
					return false;

				if (bytesTotal != 0 && expectedSize != 0 && bytesTotal != expectedSize)
				{
					fmt::print(fg(fmt::color::red), "error when downloading {0}: file size ({1}) doesn't match expected size ({2})!\n", download->filename, FormatSize(bytesTotal), FormatSize(expectedSize));
					return false;
				}

				download->downloadedSize = bytesReceived;
				OnDownloadProgress(this, download->filename, bytesReceived, bytesTotal);

				return true;
			});

			return true;
		});

		return true;
	}
}
