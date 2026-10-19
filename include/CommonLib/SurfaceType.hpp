// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SURFACETYPE_HPP
#define TSOM_COMMONLIB_SURFACETYPE_HPP

#include <NazaraUtils/Constants.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <frozen/string.h>
#include <frozen/unordered_map.h>

namespace tsom
{
	enum class SurfaceType
	{
		Dirt,
		Grass,
		Metal,

		Max = Metal
	};

	constexpr auto s_surfaceTypeFromString = frozen::make_unordered_map<frozen::string, SurfaceType>({
		{ "Dirt",  SurfaceType::Dirt  },
		{ "Grass", SurfaceType::Grass },
		{ "Metal", SurfaceType::Metal }
	});
}

#endif // TSOM_COMMONLIB_SURFACETYPE_HPP
