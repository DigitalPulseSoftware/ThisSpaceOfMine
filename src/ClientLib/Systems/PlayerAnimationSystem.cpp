// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/PlayerAnimationSystem.hpp>
#include <ClientLib/Components/PlayerAnimationComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>

namespace tsom
{
	void PlayerAnimationSystem::Update(Nz::Time elapsedTime)
	{
		auto view = m_registry.view<PlayerAnimationComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, playerAnim] : view.each())
			playerAnim.Advance(elapsedTime);
	}
}
