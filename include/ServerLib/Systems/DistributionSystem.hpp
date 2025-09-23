// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_DISTRIBUTIONSYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_DISTRIBUTIONSYSTEM_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>

namespace tsom
{
	class TSOM_SERVERLIB_API DistributionSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = -2; //< execute before TickSystem
			using Components = Nz::TypeList<DistributionComponent>;

			DistributionSystem(entt::registry& registry);
			DistributionSystem(const DistributionSystem&) = delete;
			DistributionSystem(DistributionSystem&&) = delete;
			~DistributionSystem();

			void Update(Nz::Time elapsedTime);

			DistributionSystem& operator=(const DistributionSystem&) = delete;
			DistributionSystem& operator=(DistributionSystem&&) = delete;

		private:
			void OnDistributionDestroy(entt::entity entity);
			void HandleDistributionEntity(entt::entity entity, DistributionComponent& distribution);

			struct EntityData
			{
				NazaraSlot(DistributionComponent, OnInputOutputChanged, onInputOutputChangedSlot);
			};

			tsl::hopscotch_map<entt::entity, EntityData> m_distributionEntities;
			tsl::hopscotch_set<entt::entity> m_producers;
			entt::observer m_distributionConstructObserver;
			entt::scoped_connection m_distributionDestroyConnection;
			entt::registry& m_registry;
	};
}

#include <ServerLib/Systems/DistributionSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_DISTRIBUTIONSYSTEM_HPP
