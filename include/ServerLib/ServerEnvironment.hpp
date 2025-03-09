// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SERVERENVIRONMENT_HPP
#define TSOM_SERVERLIB_SERVERENVIRONMENT_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Debug/DebugDrawInterface.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <memory>

namespace Nz
{
	class EnttWorld;
}

namespace tsom
{
	class GravityController;
	class ServerAtmosphere;
	class ServerPlayer;

	enum class ServerEnvironmentType
	{
		Planet,
		Ship
	};

	class TSOM_SERVERLIB_API ServerEnvironment
	{
		public:
			ServerEnvironment(const ServerEnvironment&) = delete;
			ServerEnvironment(ServerEnvironment&&) = delete;
			virtual ~ServerEnvironment();

			virtual Nz::Boxf ComputeBoundingBox() const = 0;

			virtual entt::handle CreateEntity() = 0;

			virtual void ForEachAtmosphere(Nz::FunctionRef<void(ServerAtmosphere*)> callback);
			virtual void ForEachAtmosphere(Nz::FunctionRef<void(const ServerAtmosphere*)> callback) const;

			template<typename F> void ForEachPlayer(F&& callback);
			template<typename F> void ForEachPlayer(F&& callback) const;

			virtual ServerAtmosphere* GetAtmosphereAtPosition(const Nz::Vector3f& position);
			virtual const GravityController* GetGravityController() const = 0;
			inline ServerInstance& GetServerInstance();
			inline ServerEnvironmentType GetType() const;
			inline Nz::EnttWorld& GetWorld();
			inline const Nz::EnttWorld& GetWorld() const;

			inline bool IsRoot() const;

			virtual void OnSave() = 0;
			virtual void OnTick(Nz::Time elapsedTime);

			void RegisterPlayer(ServerPlayer* player);
			void UnregisterPlayer(ServerPlayer* player);

			ServerEnvironment& operator=(const ServerEnvironment&) = delete;
			ServerEnvironment& operator=(ServerEnvironment&&) = delete;

			static inline ServerEnvironment* GetEnvironment(entt::handle entity);
			static inline ServerEnvironment* GetEnvironment(entt::registry& registry);

		protected:
			ServerEnvironment(ServerInstance& serverInstance, ServerEnvironmentType type, bool isRoot);

			inline void ClearEntities();

			virtual ServerAtmosphere* GetFallbackAtmosphereAtPosition(const Nz::Vector3f& position) = 0;

			std::unique_ptr<Nz::EnttWorld> m_world;
			std::unique_ptr<DebugDrawInterface> m_debugDrawer;
			Nz::Bitset<Nz::UInt64> m_registeredPlayers;
			ServerEnvironmentType m_type;
			ServerInstance& m_serverInstance;
			bool m_isRoot;
	};
}

#include <ServerLib/ServerEnvironment.inl>

#endif // TSOM_SERVERLIB_SERVERENVIRONMENT_HPP
