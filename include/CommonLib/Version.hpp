// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_VERSION_HPP
#define TSOM_COMMONLIB_VERSION_HPP

#include <CommonLib/Export.hpp>
#include <cstdint>
#include <string>

namespace tsom
{
	TSOM_COMMONLIB_API extern std::string_view BuildSystem;
	TSOM_COMMONLIB_API extern std::string_view BuildBranch;
	TSOM_COMMONLIB_API extern std::string_view BuildCommit;
	TSOM_COMMONLIB_API extern std::string_view BuildCommitDate;
	TSOM_COMMONLIB_API std::string GetBuildInfo();

	constexpr std::uint32_t BuildVersion(std::uint32_t majorVersion, std::uint32_t minorVersion, std::uint32_t patchVersion)
	{
		return majorVersion << 22 | minorVersion << 12 | patchVersion;
	}

	constexpr void DecodeVersion(std::uint32_t version, std::uint32_t& majorVersion, std::uint32_t& minorVersion, std::uint32_t& patchVersion)
	{
		majorVersion = (version >> 22) & 0x3FF;
		minorVersion = (version >> 12) & 0x3FF;
		patchVersion = (version >> 0) & 0xFFF;
	}

	constexpr std::uint32_t GameMajorVersion = 0;
	constexpr std::uint32_t GameMinorVersion = 0;
	constexpr std::uint32_t GamePatchVersion = 1;

	constexpr std::uint32_t GameVersion = BuildVersion(GameMajorVersion, GameMinorVersion, GamePatchVersion);
}

#include <CommonLib/Version.inl>

#endif
