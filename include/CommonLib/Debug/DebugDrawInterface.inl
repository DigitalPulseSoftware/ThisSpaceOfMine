// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline void DebugDrawInterface::DrawLines(Nz::UInt64 hash, float duration, std::span<const Nz::Vector3f> positions, const Nz::Color& color)
	{
		return DrawLines(hash, duration, {}, positions, color);
	}
}
