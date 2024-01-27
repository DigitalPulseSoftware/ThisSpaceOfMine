// Copyright (C) 2023 Jérôme Leclercq
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
		return fmt::format("TSOM {}.{}.{} ({})", GameMajorVersion, GameMinorVersion, GamePatchVersion, BuildBranch);
	}

#include "VersionData.hpp"

	std::uint32_t GameVersion = BuildVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);
}
