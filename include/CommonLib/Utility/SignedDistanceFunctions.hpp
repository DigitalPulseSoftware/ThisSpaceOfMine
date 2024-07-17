// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_SIGNEDDISTANCEFUNCTIONS_HPP
#define TSOM_COMMONLIB_UTILITY_SIGNEDDISTANCEFUNCTIONS_HPP

#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	inline float sdRoundBox(const Nz::Vector3f& pos, const Nz::Vector3f& halfDims, float cornerRadius);
}

#include <CommonLib/Utility/SignedDistanceFunctions.inl>

#endif // TSOM_COMMONLIB_UTILITY_SIGNEDDISTANCEFUNCTIONS_HPP
