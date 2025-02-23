// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_GASTYPE_HPP
#define TSOM_COMMONLIB_GASTYPE_HPP

#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	enum class GasType
	{
		Oxygen,

		Max = Oxygen
	};

	constexpr std::size_t GasTypeCount = static_cast<std::size_t>(GasType::Max) + 1;
}

#endif // TSOM_COMMONLIB_GASTYPE_HPP
