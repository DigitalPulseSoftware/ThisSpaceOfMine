// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_ANIMATIONSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_ANIMATIONSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API AnimationSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = 0;
			using Components = Nz::TypeList<class AnimationComponent>;

			inline AnimationSystem(entt::registry& registry);
			AnimationSystem(const AnimationSystem&) = delete;
			AnimationSystem(AnimationSystem&&) = default;
			~AnimationSystem() = default;

			void Update(Nz::Time elapsedTime);
			void UpdateAnimationStates(Nz::Time elapsedTime);

			AnimationSystem& operator=(const AnimationSystem&) = delete;
			AnimationSystem& operator=(AnimationSystem&&) = default;

		private:
			entt::registry& m_registry;
	};
}

#include <ClientLib/Systems/AnimationSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_ANIMATIONSYSTEM_HPP
