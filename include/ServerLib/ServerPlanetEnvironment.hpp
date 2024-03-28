// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Chunk.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <memory>

namespace tsom
{
	class ChunkEntities;
	class Planet;

	class TSOM_SERVERLIB_API ServerPlanetEnvironment : public ServerEnvironment
	{
		public:
			ServerPlanetEnvironment(ServerInstance& serverInstance, Nz::UInt32 seed, const Nz::Vector3ui& chunkCount);
			ServerPlanetEnvironment(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment(ServerPlanetEnvironment&&) = delete;
			~ServerPlanetEnvironment();

			inline Planet& GetPlanet();
			inline const Planet& GetPlanet() const;
			const GravityController* GetGravityController() const override;

			void OnLoad(const std::filesystem::path& loadPath) override;
			void OnSave(const std::filesystem::path& savePath) override;
			void OnTick(Nz::Time elapsedTime) override;

			ServerPlanetEnvironment& operator=(const ServerPlanetEnvironment&) = delete;
			ServerPlanetEnvironment& operator=(ServerPlanetEnvironment&&) = delete;

		private:
			std::unique_ptr<ChunkEntities> m_planetEntities;
			std::unique_ptr<Planet> m_planet;
			std::unordered_set<ChunkIndices /*chunkIndex*/> m_dirtyChunks;
	};
}

#include <ServerLib/ServerPlanetEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERPLANETENVIRONMENT_HPP
