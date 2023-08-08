// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef TSOM_COMMONLIB_PLANETENTITIES_HPP
#define TSOM_COMMONLIB_PLANETENTITIES_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Planet.hpp>
#include <NazaraUtils/Bitset.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace Nz
{
	class EnttWorld;
}

namespace tsom
{
	class Chunk;

	class TSOM_COMMONLIB_API PlanetEntities
	{
		public:
			PlanetEntities(Nz::EnttWorld& world, const Planet& planet);
			PlanetEntities(const PlanetEntities&) = delete;
			PlanetEntities(PlanetEntities&&) = delete;
			~PlanetEntities();

			void Update();

			PlanetEntities& operator=(const PlanetEntities&) = delete;
			PlanetEntities& operator=(PlanetEntities&&) = delete;

		protected:
			struct NoInit {};
			PlanetEntities(Nz::EnttWorld& world, const Planet& planet, NoInit);

			virtual void CreateChunkEntity(std::size_t chunkId, const Chunk* chunk);
			void DestroyChunkEntity(std::size_t chunkId);
			void FillChunks();
			virtual void UpdateChunkEntity(std::size_t chunkId);

			NazaraSlot(Planet, OnChunkAdded, m_onChunkAdded);
			NazaraSlot(Planet, OnChunkRemove, m_onChunkRemove);
			NazaraSlot(Planet, OnChunkUpdated, m_onChunkUpdated);

			std::vector<entt::handle> m_chunkEntities;
			Nz::EnttWorld& m_world;
			const Planet& m_planet;
			Nz::Bitset<> m_invalidatedChunks;
	};
}

#include <CommonLib/PlanetEntities.inl>

#endif // TSOM_COMMONLIB_PLANETENTITIES_HPP
