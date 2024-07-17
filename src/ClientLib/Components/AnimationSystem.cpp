// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Components/AnimationComponent.hpp>
#include <ClientLib/Systems/AnimationSystem.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>

namespace tsom
{
	void AnimationSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<AnimationComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, playerAnim] : view.each())
		{
			NazaraUnused(entity);
			playerAnim.Animate(elapsedTime);
		}
	}

	void AnimationSystem::UpdateAnimationStates(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<AnimationComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, playerAnim] : view.each())
		{
			NazaraUnused(entity);
			playerAnim.UpdateStates(elapsedTime);
		}
	}
}
