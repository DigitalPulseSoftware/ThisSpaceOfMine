// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_INTERNALCONSTANTS_HPP
#define TSOM_COMMONLIB_INTERNALCONSTANTS_HPP

#include <CommonLib/Version.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <string_view>

namespace tsom::Constants
{
	// Network constants
	constexpr Nz::UInt32 NetworkChannelCount = 3;
	constexpr Nz::UInt32 ProtocolRequiredClientVersion = BuildVersion(0, 4, 0);
	constexpr Nz::UInt32 ServerPort = 29536;
	constexpr Nz::Time TickDuration = Nz::Time::TickDuration(60);

	// Server constants
	constexpr Nz::Time SaveInterval = Nz::Time::Seconds(30);
	constexpr std::string_view SaveDirectory = "saves/chunks";

	// Serialization constants
	constexpr Nz::UInt32 ChunkBinaryVersion = 1;
}

#endif // TSOM_COMMONLIB_INTERNALCONSTANTS_HPP
