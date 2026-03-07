// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_DEBUG_DEBUGDRAWINTERFACE_HPP
#define TSOM_COMMONLIB_DEBUG_DEBUGDRAWINTERFACE_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <span>

namespace Nz
{
	class Color;
}

namespace tsom
{
	class TSOM_COMMONLIB_API DebugDrawInterface
	{
		public:
			DebugDrawInterface() = default;
			DebugDrawInterface(const DebugDrawInterface&) = delete;
			DebugDrawInterface(DebugDrawInterface&&) = delete;
			virtual ~DebugDrawInterface();

			inline void DrawLines(Nz::UInt64 hash, float duration, std::span<const Nz::Vector3f> positions, const Nz::Color& color);
			virtual void DrawLines(Nz::UInt64 hash, float duration, std::span<const Nz::UInt16> indices, std::span<const Nz::Vector3f> positions, const Nz::Color& color) = 0;

			DebugDrawInterface& operator=(const DebugDrawInterface&) = delete;
			DebugDrawInterface& operator=(DebugDrawInterface&&) = delete;
	};
}

#include <CommonLib/Debug/DebugDrawInterface.inl>

#endif // TSOM_COMMONLIB_DEBUG_DEBUGDRAWINTERFACE_HPP
