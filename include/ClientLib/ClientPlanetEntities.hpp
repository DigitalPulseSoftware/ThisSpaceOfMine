// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_CLIENTLIB_CLIENTPLANETENTITIES_HPP
#define TSOM_CLIENTLIB_CLIENTPLANETENTITIES_HPP

#include <CommonLib/PlanetEntities.hpp>
#include <ClientLib/Export.hpp>
#include <tsl/hopscotch_map.h>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
	class MaterialInstance;
	class Model;
}

namespace tsom
{
	class ClientPlanet;

	class TSOM_CLIENTLIB_API ClientPlanetEntities final : public PlanetEntities
	{
		public:
			ClientPlanetEntities(Nz::ApplicationBase& app, Nz::EnttWorld& world, ClientPlanet& planet);
			ClientPlanetEntities(const ClientPlanetEntities&) = delete;
			ClientPlanetEntities(ClientPlanetEntities&&) = delete;
			~ClientPlanetEntities() = default;

			ClientPlanetEntities& operator=(const ClientPlanetEntities&) = delete;
			ClientPlanetEntities& operator=(ClientPlanetEntities&&) = delete;

		private:
			std::shared_ptr<Nz::Model> BuildModel(const Chunk* chunk);
			void CreateChunkEntity(std::size_t chunkId, const Chunk* chunk) override;
			void UpdateChunkEntity(std::size_t chunkId) override;
			void UpdateChunkDebugCollider(std::size_t chunkId);

			std::shared_ptr<Nz::MaterialInstance> m_chunkMaterial;
	};
}

#include <ClientLib/ClientPlanetEntities.inl>

#endif // TSOM_CLIENTLIB_CLIENTPLANETENTITIES_HPP
