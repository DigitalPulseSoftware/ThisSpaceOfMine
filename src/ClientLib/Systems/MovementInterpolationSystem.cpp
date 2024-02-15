// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/MovementInterpolationSystem.hpp>
#include <ClientLib/Components/MovementInterpolationComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

namespace tsom
{
	MovementInterpolationSystem::MovementInterpolationSystem(entt::registry& registry, Nz::Time movementTickDuration, std::size_t targetMovementPointsy) :
	m_interpolatedObserver(registry, entt::collector.group<Nz::NodeComponent, MovementInterpolationComponent>(entt::exclude<Nz::DisabledComponent>)),
	m_targetMovementPoints(targetMovementPointsy),
	m_movementTickDuration(movementTickDuration),
	m_registry(registry)
	{
	}

	void MovementInterpolationSystem::Update(Nz::Time elapsedTime)
	{
		// Setup new entities
		m_interpolatedObserver.each([&](entt::entity entity)
		{
			auto& entityNode = m_registry.get<Nz::NodeComponent>(entity);
			auto& entityInterpolation = m_registry.get<MovementInterpolationComponent>(entity);

			entityInterpolation.Fill(m_targetMovementPoints, entityNode.GetPosition(), entityNode.GetRotation());
		});

		float deltaIncrement = elapsedTime.AsSeconds() / m_movementTickDuration.AsSeconds();

		// Interpolation
		auto view = m_registry.view<Nz::NodeComponent, MovementInterpolationComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, entityNode, entityInterpolation] : view.each())
		{
			Nz::Quaternionf rotation;
			Nz::Vector3f position;

			if NAZARA_LIKELY(entityInterpolation.Advance(deltaIncrement, m_targetMovementPoints, &position, &rotation))
			{
				entityNode.SetTransform(position, rotation);
			}
		}
	}
}
