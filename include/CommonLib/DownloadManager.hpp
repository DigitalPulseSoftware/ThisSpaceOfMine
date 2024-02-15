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
			inline DownloadManager(Nz::ApplicationBase& application);
			DownloadManager(const DownloadManager&) = delete;
			DownloadManager(DownloadManager&&) = delete;
			~DownloadManager() = default;

			inline bool HasDownloadInProgress() const;

			bool QueueDownload(std::string name, std::filesystem::path filepath, const std::string& downloadUrl, Nz::UInt64 expectedSize = 0, std::string expectedHash = {}, bool force = false);

			DownloadManager& operator=(const DownloadManager&) = delete;
			DownloadManager& operator=(DownloadManager&&) = delete;

			NazaraSignal(OnDownloadFailed, DownloadManager* /*downloadManager*/, const std::string& /*filename*/);
			NazaraSignal(OnDownloadFinished, DownloadManager* /*downloadManager*/, const std::string& /*filename*/);
			NazaraSignal(OnDownloadProgress, DownloadManager* /*downloadManager*/, const std::string& /*filename*/, Nz::UInt64 /*downloadedSize*/, Nz::UInt64 /*totalSize*/);

		private:
			struct Download
			{
				std::string filename;
				Nz::File file;
				Nz::SHA256Hasher hasher;
				Nz::UInt64 downloadedSize = 0;
				bool isFinished = false;
			};

			std::vector<std::shared_ptr<Download>> m_pendingDownloads;
			Nz::ApplicationBase& m_application;
			bool m_isCancelled;
	};
}

#include <CommonLib/DownloadManager.inl>

#endif // TSOM_COMMONLIB_DOWNLOADMANAGER_HPP
