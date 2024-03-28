// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <filesystem>
#include <vector>

namespace tsom
{
	class GravityController;
	class ServerInstance;
	class ServerPlayer;

	class TSOM_SERVERLIB_API ServerEnvironment
	{
		public:
			ServerEnvironment(ServerInstance& serverInstance);
			ServerEnvironment(const ServerEnvironment&) = delete;
			ServerEnvironment(ServerEnvironment&&) = delete;
			virtual ~ServerEnvironment();

			virtual entt::handle CreateEntity() = 0;

			template<typename F> void ForEachPlayer(F&& callback) const;

			virtual const GravityController* GetGravityController() const = 0;
			inline Nz::EnttWorld& GetWorld();
			inline const Nz::EnttWorld& GetWorld() const;

			virtual void OnLoad(const std::filesystem::path& loadPath) = 0;
			virtual void OnSave(const std::filesystem::path& savePath) = 0;
			virtual void OnTick(Nz::Time elapsedTime);

			inline void RegisterPlayer(ServerPlayer* player);
			inline void UnregisterPlayer(ServerPlayer* player);

			ServerEnvironment& operator=(const ServerEnvironment&) = delete;
			ServerEnvironment& operator=(ServerEnvironment&&) = delete;

		protected:
			std::vector<ServerPlayer*> m_players;
			Nz::EnttWorld m_world;
			ServerInstance& m_serverInstance;
	};
}

#include <ServerLib/ServerEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERENVIRONMENT_HPP
