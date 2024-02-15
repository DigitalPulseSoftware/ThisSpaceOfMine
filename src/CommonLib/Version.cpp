// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Version.hpp>
#include <fmt/format.h>

namespace tsom
{
	std::string GetBuildInfo()
	{
		return fmt::format("{} - {} ({}) - {}", BuildSystem, BuildBranch, BuildCommit, BuildCommitDate);
	}

	std::string GetVersionInfo()
	{
		// HEAD is the detected branch when releasing
		if (BuildBranch != "release" && BuildBranch != "HEAD")
			return fmt::format("TSOM {}.{}.{} ({})", GameMajorVersion, GameMinorVersion, GamePatchVersion, BuildBranch);
		else
			return fmt::format("TSOM {}.{}.{}", GameMajorVersion, GameMinorVersion, GamePatchVersion, BuildBranch);
	}

#include "VersionData.hpp"

	std::uint32_t GameVersion = BuildVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);
}
