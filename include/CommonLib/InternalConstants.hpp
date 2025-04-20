// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_INTERNALCONSTANTS_HPP
#define TSOM_COMMONLIB_INTERNALCONSTANTS_HPP

#include <CommonLib/Version.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <cstddef>

namespace tsom::Constants
{
	// Chunk constants
	constexpr std::size_t MaxChunkLayerCount = 4;

	// Network constants
	constexpr Nz::UInt32 NetworkChannelCount = 3;
	constexpr Nz::UInt32 ProtocolRequiredClientVersion = BuildVersion(0, 8, 1);
	constexpr Nz::Time TickDuration = Nz::Time::TickDuration(60);
	constexpr std::size_t TargetInputBufferSize = 3;

	// Serialization constants
	constexpr Nz::UInt32 ChunkBinaryVersion = 1;
}

#endif // TSOM_COMMONLIB_INTERNALCONSTANTS_HPP
