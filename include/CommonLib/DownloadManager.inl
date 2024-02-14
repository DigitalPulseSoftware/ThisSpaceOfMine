// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline DownloadManager::DownloadManager(Nz::ApplicationBase& application) :
	m_application(application)
	{
	}

	inline bool DownloadManager::HasDownloadInProgress() const
	{
		return !m_pendingDownloads.empty();
	}
}
