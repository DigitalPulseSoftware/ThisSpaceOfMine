// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SYSTEMS_TICKSYSTEM_HPP
#define TSOM_COMMONLIB_SYSTEMS_TICKSYSTEM_HPP

#include <CommonLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API TickSystem
	{
		public:
			static constexpr bool AllowConcurrent = false;
			static constexpr Nz::Int64 ExecutionOrder = -1; //< execute before physics
			using Components = Nz::TypeList<class TickComponent>;

			inline TickSystem(entt::registry& registry);
			TickSystem(const TickSystem&) = delete;
			TickSystem(TickSystem&&) = delete;
			~TickSystem() = default;

			void Update(Nz::Time elapsedTime);

			TickSystem& operator=(const TickSystem&) = delete;
			TickSystem& operator=(TickSystem&&) = delete;

		private:
			entt::registry& m_registry;
	};

}

#include <CommonLib/Systems/TickSystem.inl>

#endif // TSOM_COMMONLIB_SYSTEMS_TICKSYSTEM_HPP
