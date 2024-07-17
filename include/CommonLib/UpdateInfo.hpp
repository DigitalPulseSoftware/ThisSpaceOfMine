// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UPDATEINFO_HPP
#define TSOM_COMMONLIB_UPDATEINFO_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <semver.hpp>
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

		semver::version assetVersion;
		semver::version binaryVersion;
		DownloadInfo assets;
		DownloadInfo binaries;
		DownloadInfo updater;
	};
}

#endif // TSOM_COMMONLIB_UPDATEINFO_HPP
