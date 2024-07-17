// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline PlayerAnimationComponent::PlayerAnimationComponent(std::shared_ptr<Nz::Skeleton> skeleton, std::shared_ptr<Nz::Animation> currentAnim) :
	m_currentAnim(std::move(currentAnim)),
	m_skeleton(std::move(skeleton)),
	m_currentFrame(0),
	m_previousFrame(0),
	m_interp(0.f)
	{
	}

	inline void PlayerAnimationComponent::Advance(Nz::Time elapsedTime)
	{
		m_interp += elapsedTime.AsSeconds() * 30.f;
		while (m_interp >= 1.f)
		{
			m_previousFrame = m_currentFrame;
			m_currentFrame++;
			if (m_currentFrame >= m_currentAnim->GetFrameCount())
				m_currentFrame = 0;

			m_interp -= 1.f;
		}

		m_currentAnim->AnimateSkeleton(m_skeleton.get(), m_previousFrame, m_currentFrame, m_interp);
	}
}
