// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline DownloadManager::DownloadManager(Nz::ApplicationBase& application) :
	m_application(application)
	{
		// Download vector has to be a std::shared_ptr because the DownloadManager can be destroyed while some downloads are still actives
		m_activeDownloads = std::make_shared<DownloadList>();
	}

	inline void DownloadManager::Cancel()
	{
		for (auto& downloadPtr : *m_activeDownloads)
			downloadPtr->isCancelled = true;
	}

	inline bool DownloadManager::HasDownloadInProgress() const
	{
		return !m_activeDownloads->empty();
	}
}
