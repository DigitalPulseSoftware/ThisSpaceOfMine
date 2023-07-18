// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTPLANET_HPP
#define TSOM_CLIENTLIB_CLIENTPLANET_HPP

#include <CommonLib/Planet.hpp>
#include <ClientLib/Export.hpp>

namespace Nz
{
	class DebugDrawer;
	class GraphicalMesh;
	class JoltCollider3D;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientPlanet : public Planet
	{
		public:
			ClientPlanet(std::size_t gridDims, float tileSize, float cornerRadius);
			ClientPlanet(const ClientPlanet&) = delete;
			ClientPlanet(ClientPlanet&&) = delete;
			~ClientPlanet() = default;

			std::shared_ptr<Nz::GraphicalMesh> BuildGfxMesh();

			ClientPlanet& operator=(const ClientPlanet&) = delete;
			ClientPlanet& operator=(ClientPlanet&&) = delete;
	};
}

#include <ClientLib/ClientPlanet.inl>

#endif // TSOM_CLIENTLIB_CLIENTPLANET_HPP
