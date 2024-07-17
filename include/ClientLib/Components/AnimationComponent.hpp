// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_ANIMATIONCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_ANIMATIONCOMPONENT_HPP

#include <ClientLib/AnimationController.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <entt/entt.hpp>
#include <memory>

namespace Nz
{
	class Skeleton;
}

namespace tsom
{
	class AnimationComponent
	{
		public:
			inline AnimationComponent(std::shared_ptr<Nz::Skeleton> skeleton, std::shared_ptr<AnimationController> animationController);
			AnimationComponent(const AnimationComponent&) = default;
			AnimationComponent(AnimationComponent&&) = default;
			~AnimationComponent() = default;

			inline void Animate(Nz::Time elapsedTime);
			inline void UpdateStates(Nz::Time elapsedTime);

			AnimationComponent& operator=(const AnimationComponent&) = delete;
			AnimationComponent& operator=(AnimationComponent&&) = default;

		private:
			std::shared_ptr<AnimationController> m_animController;
			std::shared_ptr<Nz::Skeleton> m_skeleton;
	};
}

#include <ClientLib/Components/AnimationComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_ANIMATIONCOMPONENT_HPP
