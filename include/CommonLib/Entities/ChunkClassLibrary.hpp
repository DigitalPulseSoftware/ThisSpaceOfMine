// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_ENTITIES_CHUNKCLASSLIBRARY_HPP
#define TSOM_COMMONLIB_ENTITIES_CHUNKCLASSLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Entities/EntityClassLibrary.hpp>
#include <entt/fwd.hpp>
#include <memory>

namespace Nz
{
	class ApplicationBase;
	class EnttWorld;
}

namespace tsom
{
	class BlockLibrary;
	class ChunkContainer;
	struct ChunkContainerComponent;
	class ChunkEntities;
	class Planet;
	struct PlanetComponent;
	class Ship;
	struct ShipComponent;

	class TSOM_COMMONLIB_API ChunkClassLibrary : public EntityClassLibrary
	{
		public:
			inline ChunkClassLibrary(Nz::ApplicationBase& app, const BlockLibrary& blockLibrary);
			ChunkClassLibrary(const ChunkClassLibrary&) = delete;
			ChunkClassLibrary(ChunkClassLibrary&&) = delete;
			~ChunkClassLibrary() = default;

			void Register(EntityRegistry& registry) override;

			ChunkClassLibrary& operator=(const ChunkClassLibrary&) = delete;
			ChunkClassLibrary& operator=(ChunkClassLibrary&&) = delete;

		protected:
			virtual ChunkContainerComponent& InitializeChunkEntity(entt::handle entity, bool isStatic, std::shared_ptr<ChunkContainer>&& chunkContainer);
			virtual void InitializePlanetEntity(entt::handle entity, std::shared_ptr<Planet>&& planet);
			virtual void InitializeShipEntity(entt::handle entity, std::shared_ptr<Ship>&& ship);
			virtual std::unique_ptr<ChunkEntities> SetupChunkEntities(Nz::EnttWorld& world, ChunkContainer& chunkContainer, std::size_t layerIndex, bool isStatic) = 0;

			Nz::ApplicationBase& m_app;
			const BlockLibrary& m_blockLibrary;
	};
}

#include <CommonLib/Entities/ChunkClassLibrary.inl>

#endif // TSOM_COMMONLIB_ENTITIES_CHUNKCLASSLIBRARY_HPP
