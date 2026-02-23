// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Version.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	std::string GetBuildInfo()
	{
		return fmt::format("{} - {} ({}) - {}", BuildSystem, BuildBranch, BuildCommit, BuildCommitDate);
	}

	std::string GetVersionInfo()
	{
		if (IsDevVersion())
			return fmt::format("TSOM {}.{}.{} ({})", GameMajorVersion, GameMinorVersion, GamePatchVersion, BuildBranch);
		else
			return fmt::format("TSOM {}.{}.{}", GameMajorVersion, GameMinorVersion, GamePatchVersion, BuildBranch);
	}

	bool IsDevVersion()
	{
		// HEAD is the detected branch when releasing
		return BuildBranch != "release" && BuildBranch != "HEAD";
	}

#include "VersionData.hpp"

	std::uint32_t GameVersion = BuildVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);
}
