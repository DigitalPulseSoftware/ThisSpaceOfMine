// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ENTITIES_CLIENTCHUNKCLASSLIBRARY_HPP
#define TSOM_CLIENTLIB_ENTITIES_CLIENTCHUNKCLASSLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Entities/ChunkClassLibrary.hpp>

namespace tsom
{
	class ClientBlockLibrary;
	class ConfigFile;

	class TSOM_CLIENTLIB_API ClientChunkClassLibrary final : public ChunkClassLibrary
	{
		public:
			ClientChunkClassLibrary(Nz::ApplicationBase& app, ConfigFile& config, const ClientBlockLibrary& blockLibrary);

		private:
			void InitializeAsteroidEntity(entt::handle entity, std::shared_ptr<Asteroid>&& asteroid) override;
			ChunkContainerComponent& InitializeChunkEntity(entt::handle entity, bool isStatic, std::shared_ptr<ChunkContainer>&& chunkContainer) override;
			void InitializePlanetEntity(entt::handle entity, std::shared_ptr<Planet>&& planet) override;
			std::unique_ptr<ChunkEntities> SetupChunkEntities(Nz::EnttWorld& world, std::shared_ptr<ChunkContainer> chunkContainer, std::size_t layerIndex, bool isStatic) override;

			ConfigFile& m_config;
	};
}

#include <ClientLib/Entities/ClientChunkClassLibrary.inl>

#endif // TSOM_CLIENTLIB_ENTITIES_CLIENTCHUNKCLASSLIBRARY_HPP
