// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_PLAYERINDEX_HPP
#define TSOM_COMMONLIB_PLAYERINDEX_HPP

#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	using PlayerIndex = Nz::UInt16;

	constexpr PlayerIndex InvalidPlayerIndex = 0xFFFF;
}

#endif // TSOM_COMMONLIB_PLAYERINDEX_HPP
