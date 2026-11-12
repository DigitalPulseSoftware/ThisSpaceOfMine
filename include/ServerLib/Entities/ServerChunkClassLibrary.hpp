// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_ENTITIES_SERVERCHUNKCLASSLIBRARY_HPP
#define TSOM_SERVERLIB_ENTITIES_SERVERCHUNKCLASSLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Entities/ChunkClassLibrary.hpp>

namespace tsom
{
	class TSOM_SERVERLIB_API ServerChunkClassLibrary final : public ChunkClassLibrary
	{
		public:
			using ChunkClassLibrary::ChunkClassLibrary;

		private:
			void InitializeAsteroidEntity(entt::handle entity, std::shared_ptr<Asteroid>&& asteroid) override;

			std::unique_ptr<ChunkEntities> SetupChunkEntities(Nz::EnttWorld& world, std::shared_ptr<ChunkContainer> chunkContainer, std::size_t layerIndex, bool isStatic) override;
	};
}

#include <ServerLib/Entities/ServerChunkClassLibrary.inl>

#endif // TSOM_SERVERLIB_ENTITIES_SERVERCHUNKCLASSLIBRARY_HPP
