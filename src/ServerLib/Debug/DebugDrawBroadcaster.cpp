// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/Debug/DebugDrawBroadcaster.hpp>
#include <CommonLib/Protocol/Packets.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <ServerLib/ServerPlayer.hpp>

namespace tsom
{
	void DebugDrawBroadcaster::DrawLines(Nz::UInt64 hash, float duration, std::span<const Nz::UInt16> indices, std::span<const Nz::Vector3f> positions, const Nz::Color& color)
	{
		Packets::S_DebugDrawLineList debugDrawLineList;
		debugDrawLineList.uniqueHash = hash;
		debugDrawLineList.color = color;
		debugDrawLineList.duration = duration;
		debugDrawLineList.indices.assign(indices.begin(), indices.end());
		debugDrawLineList.vertices.assign(positions.begin(), positions.end());

		m_serverEnvironment->ForEachPlayer([&](ServerPlayer& player)
		{
			auto* session = player.GetSession();
			if (!session)
				return;

			auto& visibility = player.GetVisibilityHandler();

			debugDrawLineList.environmentId = visibility.GetEnvironmentId(m_serverEnvironment);
			session->SendPacket(debugDrawLineList);
		});
	}
}
