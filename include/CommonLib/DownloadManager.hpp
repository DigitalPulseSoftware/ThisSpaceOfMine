// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DOWNLOADMANAGER_HPP
#define TSOM_COMMONLIB_DOWNLOADMANAGER_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/Hash/SHA256.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class TSOM_COMMONLIB_API DownloadManager
	{
		public:
			struct Download;

			inline DownloadManager(Nz::ApplicationBase& application);
			DownloadManager(const DownloadManager&) = default;
			DownloadManager(DownloadManager&&) noexcept = default;
			~DownloadManager() = default;

			inline void Cancel();

			inline bool HasDownloadInProgress() const;

			std::shared_ptr<const Download> QueueDownload(std::filesystem::path filepath, const std::string& downloadUrl, Nz::UInt64 expectedSize = 0, std::string expectedHash = {}, bool force = false);

			DownloadManager& operator=(const DownloadManager&) = delete;
			DownloadManager& operator=(DownloadManager&&) = delete;

			struct Download
			{
				std::filesystem::path filepath;
				Nz::File file;
				Nz::SHA256Hasher hasher;
				Nz::UInt64 downloadedSize = 0;
				Nz::UInt64 totalSize;
				bool isCancelled = false;
				bool isFinished = false;

				NazaraSignal(OnDownloadFailed,   const Download& /*download*/);
				NazaraSignal(OnDownloadFinished, const Download& /*download*/);
				NazaraSignal(OnDownloadProgress, const Download& /*download*/);
			};

		private:
			using DownloadList = std::vector<std::shared_ptr<Download>>;
			std::shared_ptr<DownloadList> m_activeDownloads;
			Nz::ApplicationBase& m_application;
	};
}

#include <CommonLib/DownloadManager.inl>

#endif // TSOM_COMMONLIB_DOWNLOADMANAGER_HPP
