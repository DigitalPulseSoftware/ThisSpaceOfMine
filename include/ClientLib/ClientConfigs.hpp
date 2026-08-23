// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTCONFIGS_HPP
#define TSOM_CLIENTLIB_CLIENTCONFIGS_HPP

#include <CommonLib/ConfigFile.hpp>

namespace tsom::Config
{
	static constexpr auto Chunk_NormalSmoothAngle = ConfigFile::FloatOptionName{ "Chunk.NormalSmoothAngle" };
	static constexpr auto Graphics_FPSLimit = ConfigFile::IntegerOptionName{ "Graphics.FPSLimit" };
	static constexpr auto Graphics_SunShadowMapSize = ConfigFile::IntegerOptionName{ "Graphics.SunShadowMapSize" };
}

#endif // TSOM_CLIENTLIB_CLIENTCONFIGS_HPP
