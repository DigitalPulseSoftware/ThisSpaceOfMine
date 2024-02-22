// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/AnimationBlender.hpp>
#include <Nazara/Core/Animation.hpp>
#include <cmath>

namespace tsom
{
	void AnimationBlender::AddPoint(float value, std::shared_ptr<const Nz::Animation> animation, std::size_t sequenceIndex)
	{
		const Nz::Animation::Sequence* sequence = animation->GetSequence(sequenceIndex);
		float animSpeed = float(sequence->frameRate) / sequence->frameCount;

		m_points.push_back({
			.animation = std::move(animation),
			.sequenceIndex = sequenceIndex,
			.speed = animSpeed,
			.value = value
		});

		Refresh();
	}

	void AnimationBlender::AnimateSkeleton(Nz::Skeleton* skeleton) const
	{
		skeleton->Interpolate(m_animData[0].skeleton, m_animData[1].skeleton, m_blendingFactor);
	}

	void AnimationBlender::UpdateAnimation(Nz::Time elapsedTime)
	{
		float deltaTime = elapsedTime.AsSeconds();
		m_blendingFactor = Nz::Approach(m_blendingFactor, m_blendingFactorTarget, m_blendingFactorIncrease * deltaTime);

		const auto& pointA = m_points[m_animData[0].pointIndex];
		const auto& pointB = m_points[m_animData[1].pointIndex];
		float animationSpeed = Nz::Lerp(pointA.speed, pointB.speed, m_blendingFactor);

		m_animationProgress = std::fmod(m_animationProgress + animationSpeed * deltaTime, 1.f);

		for (std::size_t i : { 0, 1 })
		{
			const auto& point = m_points[m_animData[i].pointIndex];

			const Nz::Animation::Sequence* sequence = point.animation->GetSequence(point.sequenceIndex);

			float frameIndex = m_animationProgress * sequence->frameCount;

			std::size_t currentFrame = static_cast<std::size_t>(frameIndex);
			std::size_t nextFrame = currentFrame + 1;
			if (nextFrame >= sequence->firstFrame + sequence->frameCount)
				nextFrame = sequence->firstFrame;
			float interp = frameIndex - std::floor(frameIndex);

			point.animation->AnimateSkeleton(&m_animData[i].skeleton, currentFrame, nextFrame, interp);
		}
	}

	void AnimationBlender::Refresh()
	{
		for (std::size_t i = 0; i < m_points.size(); ++i)
		{
			if (m_points[i].value > m_currentValue)
			{
				m_animData[0].pointIndex = (i > 0) ? i - 1 : 0;
				m_animData[1].pointIndex = i;
				break;
			}
		}

		const auto& pointA = m_points[m_animData[0].pointIndex];
		const auto& pointB = m_points[m_animData[1].pointIndex];

		if NAZARA_LIKELY(m_animData[0].pointIndex != m_animData[1].pointIndex)
			m_blendingFactorTarget = std::clamp(static_cast<float>(m_currentValue - pointA.value) / (pointB.value - pointA.value), 0.f, 1.f);
		else
			m_blendingFactorTarget = 0.f;
	}
}
