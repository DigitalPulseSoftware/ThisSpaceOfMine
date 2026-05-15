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
	class ChunkEntities;

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
			virtual void InitializePlanetEntity(entt::handle entity);
			virtual void InitializeShipEntity(entt::handle entity);
			virtual std::shared_ptr<ChunkEntities> SetupChunkEntities(Nz::EnttWorld& world, entt::handle parentEntity, ChunkContainer& chunkContainer, std::size_t layerIndex);

			Nz::ApplicationBase& m_app;
			const BlockLibrary& m_blockLibrary;
	};
}

#include <CommonLib/Entities/ChunkClassLibrary.inl>

#endif // TSOM_COMMONLIB_ENTITIES_CHUNKCLASSLIBRARY_HPP
