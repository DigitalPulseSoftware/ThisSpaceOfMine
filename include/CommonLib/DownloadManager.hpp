// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DOWNLOADMANAGER_HPP
#define TSOM_COMMONLIB_DOWNLOADMANAGER_HPP

#include <CommonLib/Chunk.hpp>
#include <Nazara/Core/File.hpp>
#include <Nazara/Core/Hash/SHA256.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class DownloadManager
	{
		public:
			struct Download;

			inline DownloadManager(Nz::ApplicationBase& application);
			DownloadManager(const DownloadManager&) = delete;
			DownloadManager(DownloadManager&&) = delete;
			~DownloadManager() = default;

			inline bool HasDownloadInProgress() const;

			std::shared_ptr<const Download> QueueDownload(std::string name, const std::filesystem::path& filepath, const std::string& downloadUrl, Nz::UInt64 expectedSize = 0, std::string expectedHash = {}, bool force = false);

			DownloadManager& operator=(const DownloadManager&) = delete;
			DownloadManager& operator=(DownloadManager&&) = delete;

			struct Download
			{
				std::string name;
				Nz::File file;
				Nz::SHA256Hasher hasher;
				Nz::UInt64 downloadedSize = 0;
				Nz::UInt64 totalSize;
				bool isFinished = false;

				NazaraSignal(OnDownloadFailed,   const Download& /*download*/);
				NazaraSignal(OnDownloadFinished, const Download& /*download*/);
				NazaraSignal(OnDownloadProgress, const Download& /*download*/);
			};

		private:
			std::vector<std::shared_ptr<Download>> m_pendingDownloads;
			Nz::ApplicationBase& m_application;
			bool m_isCancelled;
	};
}

#include <CommonLib/DownloadManager.inl>

#endif // TSOM_COMMONLIB_DOWNLOADMANAGER_HPP
