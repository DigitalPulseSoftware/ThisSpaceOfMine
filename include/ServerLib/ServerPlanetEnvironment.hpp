// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <ServerLib/ServerAtmosphere.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <nlohmann/json_fwd.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/ordered_map.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>

namespace tsom
{
	class ChunkEntities;
	class Planet;

	class TSOM_SERVERLIB_API ServerPlanetEnvironment final : public ServerEnvironment
	{
		public:
			ServerPlanetEnvironment(ServerInstance& serverInstance, std::optional<Nz::UInt32> databaseId, std::string generatorName, const Nz::Vector3ui& chunkCount, std::string_view planetType, std::vector<EntityProperty>&& properties);
			ServerPlanetEnvironment(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment(ServerPlanetEnvironment&&) = delete;
			~ServerPlanetEnvironment();

			Nz::Boxf ComputeBoundingBox() const override;

			entt::handle CreateEntity() override;

			void ForEachAtmosphere(Nz::FunctionRef<void(ServerAtmosphere*)> callback) override;
			void ForEachAtmosphere(Nz::FunctionRef<void(const ServerAtmosphere*)> callback) const override;

			inline std::optional<Nz::UInt32> GetDatabaseId() const;
			const GravityController* GetGravityController() const override;
			Planet& GetPlanet();
			const Planet& GetPlanet() const;
			inline entt::handle GetPlanetEntity() const;

			void OnSave() override;

			void Update() override;

			ServerPlanetEnvironment& operator=(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment& operator=(ServerPlanetEnvironment&&) = delete;

		private:
			ServerAtmosphere* GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position) override;
			void LoadChunksFromDatabase();
			void LoadEntitiesFromDatabase();

			struct ChunkLoadingData
			{
				Nz::Vector3ui chunkCount;
				std::atomic_uint chunkLoadingCount;
				std::mutex mutex;
				std::shared_ptr<Planet> planet;
				tsl::hopscotch_map<ChunkIndices, bool> visitedChunks;
				tsl::ordered_map<ChunkIndices, DirectionMask> remainingChunks;
				std::unordered_set<ChunkIndices /*chunkIndex*/> generatedChunks;

				void HandleChunkLoaded(const ChunkIndices& chunkIndices, DirectionMask visibilityMask);
			};

			ServerAtmosphere m_atmosphere;
			std::filesystem::path m_savePath;
			std::optional<Nz::UInt32> m_databaseId;
			std::shared_ptr<ChunkLoadingData> m_chunkLoadingData;
			std::string m_generatorHash;
			std::string m_generatorName;
			std::unordered_set<ChunkIndices /*chunkIndex*/> m_dirtyChunks;
			std::unordered_map<std::string, EntityProperty> m_planetProperties;
			entt::handle m_planetEntity;
			Nz::Vector3ui m_chunkCount;
	};
}

#include <ServerLib/ServerPlanetEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
