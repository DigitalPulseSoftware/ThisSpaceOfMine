// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENT_STATES_UPDATEINFO_HPP
#define TSOM_CLIENT_STATES_UPDATEINFO_HPP

#include <optional>
#include <string>

namespace tsom
{
	struct UpdateInfo
	{
		struct DownloadInfo
		{
			std::string downloadUrl;
			std::string sha256;
			Nz::UInt64 size;
		};

		std::optional<DownloadInfo> assets;
		std::string version;
		DownloadInfo binaries;
		DownloadInfo updater;
	};
}

#endif // TSOM_CLIENT_STATES_UPDATEINFO_HPP
