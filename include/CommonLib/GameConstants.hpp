// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_GAMECONSTANTS_HPP
#define TSOM_COMMONLIB_GAMECONSTANTS_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <Nazara/Core/Time.hpp>

namespace tsom::Constants
{
	constexpr Nz::Time TickDuration = Nz::Time::TickDuration(60);
	constexpr Nz::UInt16 ServerPort = 29536;
}

#endif
