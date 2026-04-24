// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void DebugDrawInterface::DrawBox(Nz::UInt64 hash, float duration, const Nz::Boxf& box, const Nz::Color& color)
	{
		std::array<Nz::Vector3f, 8> positions = box.GetCorners();
		std::array<Nz::UInt16, 24> indices = {
			Nz::SafeCaster(Nz::BoxCorner::LeftBottomNear), Nz::SafeCaster(Nz::BoxCorner::RightBottomNear),
			Nz::SafeCaster(Nz::BoxCorner::LeftBottomNear), Nz::SafeCaster(Nz::BoxCorner::LeftTopNear),
			Nz::SafeCaster(Nz::BoxCorner::LeftBottomNear), Nz::SafeCaster(Nz::BoxCorner::LeftBottomFar),
			Nz::SafeCaster(Nz::BoxCorner::RightTopFar), Nz::SafeCaster(Nz::BoxCorner::LeftTopFar),
			Nz::SafeCaster(Nz::BoxCorner::RightTopFar), Nz::SafeCaster(Nz::BoxCorner::RightBottomFar),
			Nz::SafeCaster(Nz::BoxCorner::RightTopFar), Nz::SafeCaster(Nz::BoxCorner::RightTopNear),
			Nz::SafeCaster(Nz::BoxCorner::LeftBottomFar), Nz::SafeCaster(Nz::BoxCorner::RightBottomFar),
			Nz::SafeCaster(Nz::BoxCorner::LeftBottomFar), Nz::SafeCaster(Nz::BoxCorner::LeftTopFar),
			Nz::SafeCaster(Nz::BoxCorner::LeftTopNear), Nz::SafeCaster(Nz::BoxCorner::RightTopNear),
			Nz::SafeCaster(Nz::BoxCorner::LeftTopNear), Nz::SafeCaster(Nz::BoxCorner::LeftTopFar),
			Nz::SafeCaster(Nz::BoxCorner::RightBottomNear), Nz::SafeCaster(Nz::BoxCorner::RightTopNear),
			Nz::SafeCaster(Nz::BoxCorner::RightBottomNear), Nz::SafeCaster(Nz::BoxCorner::RightBottomFar)
		};

		return DrawLines(hash, duration, indices, positions, color);
	}

	inline void DebugDrawInterface::DrawLines(Nz::UInt64 hash, float duration, std::span<const Nz::Vector3f> positions, const Nz::Color& color)
	{
		return DrawLines(hash, duration, {}, positions, color);
	}
}
