// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_DEBUG_DEBUGDRAWBROADCASTER_HPP
#define TSOM_SERVERLIB_DEBUG_DEBUGDRAWBROADCASTER_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Debug/DebugDrawInterface.hpp>

namespace tsom
{
	class ServerEnvironment;

	class TSOM_SERVERLIB_API DebugDrawBroadcaster : public DebugDrawInterface
	{
		public:
			inline DebugDrawBroadcaster(ServerEnvironment* environment);
			DebugDrawBroadcaster(const DebugDrawBroadcaster&) = delete;
			DebugDrawBroadcaster(DebugDrawBroadcaster&&) = delete;
			~DebugDrawBroadcaster() = default;

			void DrawLines(Nz::UInt64 hash, float duration, std::span<const Nz::UInt16> indices, std::span<const Nz::Vector3f> positions, const Nz::Color& color) override;

			DebugDrawBroadcaster& operator=(const DebugDrawBroadcaster&) = delete;
			DebugDrawBroadcaster& operator=(DebugDrawBroadcaster&&) = delete;

		private:
			ServerEnvironment* m_serverEnvironment;
	};
}

#include <ServerLib/Debug/DebugDrawBroadcaster.inl>

#endif // TSOM_SERVERLIB_DEBUG_DEBUGDRAWBROADCASTER_HPP
