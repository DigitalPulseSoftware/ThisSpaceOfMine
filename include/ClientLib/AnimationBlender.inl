// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline AnimationBlender::AnimationBlender(const Nz::Skeleton& referenceSkeleton) :
	m_animationProgress(0.f),
	m_blendingFactor(0.f),
	m_blendingFactorIncrease(1.f),
	m_blendingFactorTarget(0.f)
	{
		for (AnimationData& animData : m_animData)
			animData.skeleton = referenceSkeleton;
	}

	inline void AnimationBlender::UpdateValue(float value)
	{
		m_currentValue = value;

		Refresh();
	}

	inline void AnimationBlender::UpdateBlendingFactorIncrease(float increasePerSecond)
	{
		m_blendingFactorIncrease = increasePerSecond;
	}
}
