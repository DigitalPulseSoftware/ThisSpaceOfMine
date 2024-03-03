// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_SHIPSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_SHIPSYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ShipSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = 0;
			using Components = Nz::TypeList<class ShipComponent>;

			inline ShipSystem(entt::registry& registry);
			ShipSystem(const ShipSystem&) = delete;
			ShipSystem(ShipSystem&&) = delete;
			~ShipSystem() = default;

			void Update(Nz::Time elapsedTime);

			ShipSystem& operator=(const ShipSystem&) = delete;
			ShipSystem& operator=(ShipSystem&&) = delete;

		private:
			entt::registry& m_registry;
	};
}

#include <CommonLib/Systems/ShipSystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_SHIPSYSTEM_HPP
