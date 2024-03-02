// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_PLAYERANIMATIONCONTROLLER_HPP
#define TSOM_CLIENTLIB_PLAYERANIMATIONCONTROLLER_HPP

#include <ClientLib/AnimationController.hpp>
#include <Nazara/Core/Animation.hpp>
#include <Nazara/Core/AnimationBlender.hpp>
#include <Nazara/Core/Skeleton.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <entt/entt.hpp>
#include <memory>

namespace tsom
{
	struct PlayerAnimationAssets
	{
		std::shared_ptr<Nz::Animation> idleAnimation;
		std::shared_ptr<Nz::Animation> runningAnimation;
		std::shared_ptr<Nz::Animation> walkingAnimation;
		Nz::Skeleton referenceSkeleton;
	};

	class TSOM_CLIENTLIB_API PlayerAnimationController : public AnimationController
	{
		public:
			PlayerAnimationController(entt::handle entity, std::shared_ptr<PlayerAnimationAssets> animationAssets);
			PlayerAnimationController(const PlayerAnimationController&) = delete;
			PlayerAnimationController(PlayerAnimationController&&) = delete;
			~PlayerAnimationController() = default;

			void Animate(Nz::Skeleton& skeleton, Nz::Time elapsedTime) override;

			void UpdateStates(Nz::Time elapsedTime) override;

			PlayerAnimationController& operator=(const PlayerAnimationController&) = delete;
			PlayerAnimationController& operator=(PlayerAnimationController&&) = delete;

		private:
			std::shared_ptr<PlayerAnimationAssets> m_animationAssets;
			entt::handle m_entity;
			Nz::AnimationBlender m_idleRunBlender;
			Nz::Vector3f m_playerPosition;
			float m_playerVelocity;
	};
}

#include <ClientLib/PlayerAnimationController.inl>

#endif // TSOM_CLIENTLIB_PLAYERANIMATIONCONTROLLER_HPP
