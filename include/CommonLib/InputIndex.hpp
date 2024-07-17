// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_INPUTINDEX_HPP
#define TSOM_COMMONLIB_INPUTINDEX_HPP

#include <NazaraUtils/Prerequisites.hpp>

namespace tsom
{
	using InputIndex = Nz::UInt8;

	inline bool IsInputMoreRecent(InputIndex a, InputIndex b);
}

#include <CommonLib/InputIndex.inl>

#endif // TSOM_COMMONLIB_INPUTINDEX_HPP
