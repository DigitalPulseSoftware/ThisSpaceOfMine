// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/EnvironmentTransform.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/Node.hpp>
#include <tsl/hopscotch_map.h>
#include <filesystem>
#include <vector>

namespace tsom
{
	class GravityController;
	class ServerPlayer;

	class TSOM_SERVERLIB_API ServerEnvironment
	{
		public:
			ServerEnvironment(ServerInstance& serverInstance);
			ServerEnvironment(const ServerEnvironment&) = delete;
			ServerEnvironment(ServerEnvironment&&) = delete;
			virtual ~ServerEnvironment();

			void Connect(ServerEnvironment& environment, const EnvironmentTransform& transform);
			void Disconnect(ServerEnvironment& environment);

			virtual entt::handle CreateEntity() = 0;

			template<typename F> void ForEachConnectedEnvironment(F&& callback) const;
			template<typename F> void ForEachPlayer(F&& callback);
			template<typename F> void ForEachPlayer(F&& callback) const;

			inline bool GetEnvironmentTransformation(ServerEnvironment& targetEnv, EnvironmentTransform* transform) const;
			virtual const GravityController* GetGravityController() const = 0;
			inline Nz::EnttWorld& GetWorld();
			inline const Nz::EnttWorld& GetWorld() const;

			virtual void OnLoad(const std::filesystem::path& loadPath) = 0;
			virtual void OnSave(const std::filesystem::path& savePath) = 0;
			virtual void OnTick(Nz::Time elapsedTime);

			void RegisterPlayer(ServerPlayer* player);
			void UnregisterPlayer(ServerPlayer* player);

			void UpdateConnectedTransform(ServerEnvironment& environment, const EnvironmentTransform& transform);

			ServerEnvironment& operator=(const ServerEnvironment&) = delete;
			ServerEnvironment& operator=(ServerEnvironment&&) = delete;

		protected:
			tsl::hopscotch_map<ServerEnvironment*, EnvironmentTransform> m_connectedEnvironments;
			Nz::Bitset<Nz::UInt64> m_registeredPlayers;
			Nz::EnttWorld m_world;
			ServerInstance& m_serverInstance;
	};
}

#include <ServerLib/ServerEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERENVIRONMENT_HPP
