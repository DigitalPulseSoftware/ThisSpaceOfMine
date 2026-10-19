// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_PLAYERANIMATIONCONTROLLER_HPP
#define TSOM_CLIENTLIB_PLAYERANIMATIONCONTROLLER_HPP

#include <ClientLib/AnimationController.hpp>
#include <CommonLib/SurfaceType.hpp>
#include <CommonLib/Utility/AverageValues.hpp>
#include <Nazara/Core/AnimationBlender.hpp>
#include <Nazara/Core/Skeleton.hpp>
#include <Nazara/Math/Vector3.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <entt/entt.hpp>
#include <memory>
#include <vector>

namespace Nz
{
	class Animation;
	class Model;
	class SoundBuffer;
}

namespace tsom
{
	struct PlayerAssets
	{
		std::shared_ptr<Nz::Animation> idleAnimation;
		std::shared_ptr<Nz::Animation> runningAnimation;
		std::shared_ptr<Nz::Animation> walkingAnimation;
		std::shared_ptr<Nz::Model> model;
		Nz::EnumArray<SurfaceType, std::vector<std::shared_ptr<Nz::SoundBuffer>>> footstepSounds;
		Nz::Skeleton referenceSkeleton;
	};

	class ClientBlockLibrary;

	class TSOM_CLIENTLIB_API PlayerAnimationController : public AnimationController
	{
		public:
			PlayerAnimationController(ClientBlockLibrary& blockLibrary, entt::handle entity, std::shared_ptr<PlayerAssets> playerAssets);
			PlayerAnimationController(const PlayerAnimationController&) = delete;
			PlayerAnimationController(PlayerAnimationController&&) = delete;
			~PlayerAnimationController() = default;

			void Animate(Nz::Skeleton& skeleton, Nz::Time elapsedTime) override;

			void UpdateStates(Nz::Time elapsedTime) override;

			PlayerAnimationController& operator=(const PlayerAnimationController&) = delete;
			PlayerAnimationController& operator=(PlayerAnimationController&&) = delete;

		private:
			std::shared_ptr<PlayerAssets> m_playerAssets;
			entt::handle m_entity;
			Nz::AnimationBlender m_idleRunBlender;
			Nz::Time m_footstepAccumulator;
			Nz::Vector3f m_playerPosition;
			AverageValues<float> m_playerVelocities;
			ClientBlockLibrary& m_blockLibrary;
	};
}

#include <ClientLib/PlayerAnimationController.inl>

#endif // TSOM_CLIENTLIB_PLAYERANIMATIONCONTROLLER_HPP
