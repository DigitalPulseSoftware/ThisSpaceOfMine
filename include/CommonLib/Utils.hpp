// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILS_HPP
#define TSOM_COMMONLIB_UTILS_HPP

#include <CommonLib/Export.hpp>
#include <string>

namespace tsom
{
	TSOM_COMMONLIB_API std::string ByteToString(Nz::UInt64 bytes, bool speed = false);
}

#include <CommonLib/Utils.inl>

#endif // TSOM_COMMONLIB_UTILS_HPP
