// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline AnimationComponent::AnimationComponent(std::shared_ptr<Nz::Skeleton> skeleton, std::shared_ptr<AnimationController> animationController) :
	m_animController(std::move(animationController)),
	m_skeleton(std::move(skeleton))
	{
	}

	inline void AnimationComponent::Animate(Nz::Time elapsedTime)
	{
		m_animController->Animate(*m_skeleton, elapsedTime);
	}

	inline void AnimationComponent::UpdateStates(Nz::Time elapsedTime)
	{
		m_animController->UpdateStates(elapsedTime);
	}
}
