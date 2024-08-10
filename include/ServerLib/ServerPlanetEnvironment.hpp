// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <entt/entt.hpp>
#include <filesystem>
#include <memory>

namespace tsom
{
	class ChunkEntities;
	class Planet;

	class TSOM_SERVERLIB_API ServerPlanetEnvironment final : public ServerEnvironment
	{
		public:
			ServerPlanetEnvironment(ServerInstance& serverInstance, std::filesystem::path savePath, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount);
			ServerPlanetEnvironment(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment(ServerPlanetEnvironment&&) = delete;
			~ServerPlanetEnvironment();

			entt::handle CreateEntity() override;

			const GravityController* GetGravityController() const override;
			Planet& GetPlanet();
			const Planet& GetPlanet() const;
			inline entt::handle GetPlanetEntity() const;

			void OnSave() override;

			ServerPlanetEnvironment& operator=(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment& operator=(ServerPlanetEnvironment&&) = delete;

		private:
			void LoadFromDirectory();

			std::filesystem::path m_savePath;
			std::unordered_set<ChunkIndices /*chunkIndex*/> m_dirtyChunks;
			entt::handle m_planetEntity;
	};
}

#include <ServerLib/ServerPlanetEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
