// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_SPACETRANSFORMATIONS_HPP
#define TSOM_COMMONLIB_UTILITY_SPACETRANSFORMATIONS_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <utility>

namespace tsom::SpaceTransformations
{
	inline Nz::Vector3d ToGlobalCoordinates(const Nz::Vector3i32& cellCoordinates, const Nz::Vector3f& localCoordinates);
	inline std::pair<Nz::Vector3i32, Nz::Vector3f> ToLocalCoordinates(const Nz::Vector3d& globalCoords);
}

#include <CommonLib/Utility/SpaceTransformations.inl>

#endif // TSOM_COMMONLIB_UTILITY_SPACETRANSFORMATIONS_HPP
