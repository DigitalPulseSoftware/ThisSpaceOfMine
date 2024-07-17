// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

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
