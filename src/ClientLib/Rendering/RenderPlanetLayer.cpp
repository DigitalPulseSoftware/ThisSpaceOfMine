// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Rendering/RenderPlanetLayer.hpp>
#include <Nazara/Graphics/Algorithm.hpp>
#include <Nazara/Graphics/RenderQueueRegistry.hpp>
#include <Nazara/Graphics/WorldInstance.hpp>

namespace tsom
{
	Nz::UInt64 RenderPlanetLayer::ComputeSortingScore(const Nz::Frustumf& frustum, const Nz::RenderQueueRegistry& registry) const
	{
		Nz::UInt64 layerIndex = registry.FetchLayerIndex(m_renderLayer);
		Nz::UInt64 elementType = GetElementType();

		if (m_materialFlags.Test(Nz::MaterialPassFlag::SortByDistance))
		{
			// Same as Nazara
			Nz::UInt64 matFlags = 1;

			float distanceNear = frustum.GetPlane(Nz::FrustumPlane::Near).SignedDistance(m_worldInstance.GetWorldMatrix().GetTranslation());
			Nz::UInt64 reversedDistance = Nz::DistanceAsSortKey(distanceNear);

			// Transparent RQ index:
			// - Layer (8bits)
			// - Sorted by distance flag (1bit)
			// - Reversed distance to near plane (32bits) - back to front to handle transparency
			// - Unused (23bits)

			return (layerIndex & 0xFF) << 56 |
			       (matFlags)          << 55 |
			       (reversedDistance)  << 23;
		}
		else
		{
			Nz::UInt64 matFlags = 0;

			float distanceNear = frustum.GetPlane(Nz::FrustumPlane::Near).SignedDistance(m_worldInstance.GetWorldMatrix().GetTranslation());
			Nz::UInt64 distance = ~Nz::DistanceAsSortKey(distanceNear);
			
			// Planet RQ key:
			// - Layer (8bits)
			// - Sorted by distance flag (1bit)
			// - Element type (4bits)
			// - Distance to near plane (32bits) - front to back to have near planets occluding far planets
			// - Unused (19bits)
			return (layerIndex & 0xFF)              << 56 |
			       (matFlags)                       << 55 |
			       (elementType & 0xF)              << 51 |
			       (distance)                       << 19;
		}
	}

	void RenderPlanetLayer::Register(Nz::RenderQueueRegistry& registry) const
	{
		registry.RegisterLayer(m_renderLayer);
	}
}
