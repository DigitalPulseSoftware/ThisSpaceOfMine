// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	constexpr Direction DirectionFromNormal(const Nz::Vector3f& outsideNormal)
	{
		Direction closestDir;
		float closestDirDot = -1.f;
		for (auto&& [direction, normal] : s_dirNormals.iter_kv())
		{
			if (float dot = outsideNormal.DotProduct(normal); dot >= closestDirDot)
			{
				closestDir = direction;
				closestDirDot = dot;
			}
		}

		return closestDir;
	}
}
