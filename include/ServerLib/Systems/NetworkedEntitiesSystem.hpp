// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_NETWORKEDENTITIESSYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_NETWORKEDENTITIESSYSTEM_HPP

#include <ServerLib/Export.hpp>
#include <ServerLib/SessionVisibilityHandler.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/FunctionRef.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_set.h>

namespace tsom
{
	class ServerEnvironment;

	class TSOM_SERVERLIB_API NetworkedEntitiesSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = 10'000'000;
			using Components = Nz::TypeList<class NetworkedComponent>;

			NetworkedEntitiesSystem(entt::registry& registry, ServerEnvironment& environment);
			NetworkedEntitiesSystem(const NetworkedEntitiesSystem&) = delete;
			NetworkedEntitiesSystem(NetworkedEntitiesSystem&&) = delete;
			~NetworkedEntitiesSystem() = default;

			void CreateAllEntities(SessionVisibilityHandler& visibility) const;

			void ForEachVisibility(const Nz::FunctionRef<void(SessionVisibilityHandler& visibility)>& functor);

			void Update(Nz::Time elapsedTime);

			NetworkedEntitiesSystem& operator=(const NetworkedEntitiesSystem&) = delete;
			NetworkedEntitiesSystem& operator=(NetworkedEntitiesSystem&&) = delete;

		private:
			SessionVisibilityHandler::CreateEntityData BuildCreateEntityData(entt::entity entity) const;
			void CreateEntity(SessionVisibilityHandler& visibility, entt::handle entity, const SessionVisibilityHandler::CreateEntityData& createData) const;
			void OnNetworkedDestroy(entt::registry& registry, entt::entity entity);

			tsl::hopscotch_set<entt::entity> m_movingEntities;
			tsl::hopscotch_set<entt::entity> m_networkedEntities;
			entt::observer m_networkedConstructObserver;
			entt::scoped_connection m_disabledConstructConnection;
			entt::scoped_connection m_networkedDestroyConnection;
			entt::scoped_connection m_nodeDestroyConnection;
			entt::registry& m_registry;
			ServerEnvironment& m_environment;
	};
}

#include <ServerLib/Systems/NetworkedEntitiesSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_NETWORKEDENTITIESSYSTEM_HPP
