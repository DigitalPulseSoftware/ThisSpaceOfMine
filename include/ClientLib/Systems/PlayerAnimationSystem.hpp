// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_PLAYERANIMATIONSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_PLAYERANIMATIONSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API PlayerAnimationSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = 0;
			using Components = Nz::TypeList<class PlayerAnimationComponent>;

			inline PlayerAnimationSystem(entt::registry& registry);
			PlayerAnimationSystem(const PlayerAnimationSystem&) = delete;
			PlayerAnimationSystem(PlayerAnimationSystem&&) = default;
			~PlayerAnimationSystem() = default;

			void Update(Nz::Time elapsedTime);

			PlayerAnimationSystem& operator=(const PlayerAnimationSystem&) = delete;
			PlayerAnimationSystem& operator=(PlayerAnimationSystem&&) = default;

		private:
			entt::registry& m_registry;
	};
}

#include <ClientLib/Systems/PlayerAnimationSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_PLAYERANIMATIONSYSTEM_HPP
