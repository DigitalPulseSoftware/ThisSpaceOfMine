// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_GAMEINTERFACE_HPP
#define TSOM_CLIENTLIB_GAMEINTERFACE_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <entt/entt.hpp>
#include <optional>

namespace Nz
{
	class DebugDrawer;
	class EnttWorld;
}

namespace tsom
{
	struct BlockSelectionBar;
	class EntityRegistry;
	class NetworkSession;

	class TSOM_CLIENTLIB_API GameInterface
	{
		public:
			struct RaycastResult;

			GameInterface() = default;
			GameInterface(const GameInterface&) = delete;
			GameInterface(GameInterface&&) = delete;
			~GameInterface() = default;

			virtual BlockSelectionBar* GetBlockSelectionBar() const = 0;
			virtual Nz::DebugDrawer* GetDebugDrawer() = 0;
			virtual const EntityRegistry& GetEntityRegistry() const = 0;
			virtual NetworkSession* GetNetworkSession() = 0;
			virtual Nz::EnttWorld& GetWorld() = 0;
			virtual std::optional<RaycastResult> RaycastQuery() const = 0;
			virtual void UpdateMouseLock() = 0;

			GameInterface& operator=(const GameInterface&) = delete;
			GameInterface& operator=(GameInterface&&) = delete;

			struct RaycastResult
			{
				entt::handle hitEntity;
				Nz::Vector3f hitPos;
				Nz::Vector3f hitNormal;
				Nz::UInt32 subShapeID;
			};

		private:
	};
}

#include <ClientLib/GameInterface.inl>

#endif // TSOM_CLIENTLIB_GAMEINTERFACE_HPP
