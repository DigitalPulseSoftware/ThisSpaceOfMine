// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMMONCONFIGS_HPP
#define TSOM_COMMONLIB_COMMONCONFIGS_HPP

#include <CommonLib/ConfigFile.hpp>

namespace tsom::Config
{
	static constexpr auto Api_Url = ConfigFile::StringOptionName{ "Api.Url" };
}

#endif // TSOM_COMMONLIB_COMMONCONFIGS_HPP
