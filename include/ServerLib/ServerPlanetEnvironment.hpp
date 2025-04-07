// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <ServerLib/ServerAtmosphere.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <filesystem>
#include <optional>

namespace tsom
{
	class ChunkEntities;
	class Planet;

	class TSOM_SERVERLIB_API ServerPlanetEnvironment final : public ServerEnvironment
	{
		public:
			ServerPlanetEnvironment(ServerInstance& serverInstance, std::optional<Nz::UInt32> databaseId, std::string generatorName, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount, float cellSize, float cornerRadius = 16.f);
			ServerPlanetEnvironment(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment(ServerPlanetEnvironment&&) = delete;
			~ServerPlanetEnvironment();

			Nz::Boxf ComputeBoundingBox() const override;

			entt::handle CreateEntity() override;

			void ForEachAtmosphere(Nz::FunctionRef<void(ServerAtmosphere*)> callback) override;
			void ForEachAtmosphere(Nz::FunctionRef<void(const ServerAtmosphere*)> callback) const override;

			const GravityController* GetGravityController() const override;
			Planet& GetPlanet();
			const Planet& GetPlanet() const;
			inline entt::handle GetPlanetEntity() const;

			void OnSave() override;

			ServerPlanetEnvironment& operator=(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment& operator=(ServerPlanetEnvironment&&) = delete;

		private:
			ServerAtmosphere* GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position) override;
			void LoadChunksFromDatabase();
			void LoadEntitiesFromDatabase();

			ServerAtmosphere m_atmosphere;
			std::filesystem::path m_savePath;
			std::optional<Nz::UInt32> m_databaseId;
			std::unordered_set<ChunkIndices /*chunkIndex*/> m_dirtyChunks;
			entt::handle m_planetEntity;
	};
}

#include <ServerLib/ServerPlanetEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
