// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERCONSTANTS_HPP
#define TSOM_SERVERLIB_SERVERCONSTANTS_HPP

#include <Nazara/Core/Time.hpp>

namespace tsom::Constants
{
	constexpr Nz::Time PlayerTokenRefreshWindow = Nz::Time::Seconds(15);
}

#endif // TSOM_SERVERLIB_SERVERCONSTANTS_HPP
