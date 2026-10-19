// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cmath>

namespace tsom::SpaceTransformations
{
	inline Nz::Vector3d ToGlobalCoordinates(const Nz::Vector3i32& cellCoordinates, const Nz::Vector3f& localCoordinates)
	{
		constexpr double SpaceCellSize_d = Constants::SpaceCellSize;

		return Nz::Vector3d(cellCoordinates) * SpaceCellSize_d + Nz::Vector3d(localCoordinates);
	}

	inline std::pair<Nz::Vector3i32, Nz::Vector3f> ToLocalCoordinates(const Nz::Vector3d& globalCoords)
	{
		constexpr double SpaceCellSize_d = Constants::SpaceCellSize;

		// Positive when value is positive on this axis
		Nz::Vector3d offset = Nz::Vector3d(SpaceCellSize_d * 0.5);
		offset *= Nz::Vector3d::Apply(globalCoords, [](double value) { return std::copysign(1.0, value); });

		Nz::Vector3d offsetCoords = globalCoords + offset;
		Nz::Vector3d cellCoordinates = offsetCoords / Nz::Vector3d(SpaceCellSize_d);
		Nz::Vector3d localCoordinates = offsetCoords % Nz::Vector3d(SpaceCellSize_d) - offset;

		return { Nz::Vector3i32(cellCoordinates), Nz::Vector3f(localCoordinates) };
	}
}
