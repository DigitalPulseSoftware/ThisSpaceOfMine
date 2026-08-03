// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SYSTEMS_DISTRIBUTIONSYSTEM_HPP
#define TSOM_SERVERLIB_SYSTEMS_DISTRIBUTIONSYSTEM_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Components/DistributionComponent.hpp>
#include <Nazara/Core/EnttObserver.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>

namespace Nz
{
	class DisabledComponent;
}

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
			~DistributionSystem() = default;

			void Update(Nz::Time elapsedTime);

			DistributionSystem& operator=(const DistributionSystem&) = delete;
			DistributionSystem& operator=(DistributionSystem&&) = delete;

		private:
			void HandleDistributionEntity(entt::entity entity, DistributionComponent& distribution);

			struct EntityData
			{
				NazaraSlot(DistributionComponent, OnOutputChanged, onOutputChangedSlot);
			};

			entt::storage<void> m_producers;
			Nz::EnttObserver<Nz::TypeList<DistributionComponent>, Nz::TypeList<Nz::DisabledComponent>, EntityData> m_distributionObserver;
			Nz::EnumArray<DistributionType, DistributionQuantity> m_cachedValues;
			Nz::Time m_tickAccumulator;
			entt::registry& m_registry;
	};
}

#include <ServerLib/Systems/DistributionSystem.inl>

#endif // TSOM_SERVERLIB_SYSTEMS_DISTRIBUTIONSYSTEM_HPP
