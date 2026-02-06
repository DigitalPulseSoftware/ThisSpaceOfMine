// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_BLOCKINDEX_HPP
#define TSOM_COMMONLIB_BLOCKINDEX_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <NazaraUtils/Constants.hpp>

namespace tsom
{
	using BlockIndex = Nz::UInt16;

	constexpr BlockIndex EmptyBlockIndex = 0;
	constexpr BlockIndex InvalidBlockIndex = Nz::MaxValue();
}

#include <CommonLib/BlockIndex.inl>

#endif // TSOM_COMMONLIB_BLOCKINDEX_HPP
