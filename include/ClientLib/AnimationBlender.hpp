// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ANIMATIONBLENDER_HPP
#define TSOM_CLIENTLIB_ANIMATIONBLENDER_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Skeleton.hpp>
#include <Nazara/Core/Time.hpp>
#include <vector>

namespace Nz
{
	class Animation;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API AnimationBlender
	{
		public:
			inline AnimationBlender(const Nz::Skeleton& referenceSkeleton);
			AnimationBlender(const AnimationBlender&) = delete;
			AnimationBlender(AnimationBlender&&) noexcept = default;
			~AnimationBlender() = default;

			void AddPoint(float value, std::shared_ptr<const Nz::Animation> animation, std::size_t sequenceIndex = 0);
			void AnimateSkeleton(Nz::Skeleton* skeleton) const;

			void UpdateAnimation(Nz::Time elapsedTime);
			inline void UpdateBlendingFactorIncrease(float increasePerSecond);
			inline void UpdateValue(float value);

			AnimationBlender& operator=(const AnimationBlender&) = delete;
			AnimationBlender& operator=(AnimationBlender&&) noexcept = default;

		private:
			void Refresh();

			struct AnimationData
			{
				Nz::Skeleton skeleton;
				std::size_t pointIndex = 0;
			};

			struct Point
			{
				std::shared_ptr<const Nz::Animation> animation;
				std::size_t sequenceIndex;
				float speed;
				float value;
			};

			std::array<AnimationData, 2> m_animData;
			std::vector<Point> m_points;
			float m_animationProgress;
			float m_blendingFactorIncrease;
			float m_blendingFactorTarget;
			float m_currentValue;
			float m_blendingFactor;
	};
}

#include <ClientLib/AnimationBlender.inl>

#endif // TSOM_CLIENTLIB_ANIMATIONBLENDER_HPP
