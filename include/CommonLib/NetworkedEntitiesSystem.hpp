// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_NETWORKEDENTITIESYSTEM_HPP
#define TSOM_COMMONLIB_NETWORKEDENTITIESYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <Nazara/Core/Time.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_set.h>

namespace tsom
{
	class SessionVisibilityHandler;
	class ServerWorld;

	class TSOM_COMMONLIB_API NetworkedEntitiesSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = 10'000'000;
			using Components = Nz::TypeList<class NetworkedComponent>;

			NetworkedEntitiesSystem(ServerWorld& world, entt::registry& registry);
			NetworkedEntitiesSystem(const NetworkedEntitiesSystem&) = delete;
			NetworkedEntitiesSystem(NetworkedEntitiesSystem&&) = delete;
			~NetworkedEntitiesSystem() = default;

			void ForEachVisibility(const Nz::FunctionRef<void(SessionVisibilityHandler& visibility)>& functor);

			void Update(Nz::Time elapsedTime);

			NetworkedEntitiesSystem& operator=(const NetworkedEntitiesSystem&) = delete;
			NetworkedEntitiesSystem& operator=(NetworkedEntitiesSystem&&) = delete;

		private:
			void OnNetworkedDestroy(entt::registry& registry, entt::entity entity);

			tsl::hopscotch_set<entt::entity> m_movingEntities;
			entt::observer m_networkedConstructObserver;
			entt::scoped_connection m_disabledConstructConnection;
			entt::scoped_connection m_networkedDestroyConnection;
			entt::scoped_connection m_nodeDestroyConnection;
			entt::registry& m_registry;
			ServerWorld& m_world;
	};
}

#include <CommonLib/NetworkedEntitiesSystem.inl>

#endif
