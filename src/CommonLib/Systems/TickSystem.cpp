// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Systems/TickSystem.hpp>
#include <CommonLib/Components/TickComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	void TickSystem::Update(Nz::Time elapsedTime)
	{
		auto tickView = m_registry.view<TickComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, tickComponent] : tickView.each())
		{
			tickComponent.timeBeforeTick -= elapsedTime;
			if (tickComponent.timeBeforeTick > Nz::Time::Zero())
				continue;

			tickComponent.onTick(entt::handle(m_registry, entity));

			// Increase by tickrate after calling onTick in change onTick changes the tickrate value
			tickComponent.timeBeforeTick += tickComponent.tickRate;
		}
	}
}
