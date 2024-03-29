// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/PlayerAnimationController.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Core/Skeleton.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Platform/Keyboard.hpp>
#include <entt/entt.hpp>
#include <fmt/core.h>

namespace tsom
{
	PlayerAnimationController::PlayerAnimationController(entt::handle entity, std::shared_ptr<PlayerAnimationAssets> animationAssets) :
	m_animationAssets(std::move(animationAssets)),
	m_entity(entity),
	m_idleRunBlender(m_animationAssets->referenceSkeleton),
	m_playerVelocities(20)
	{
		m_idleRunBlender.AddPoint(0.f, m_animationAssets->idleAnimation);
		m_idleRunBlender.AddPoint(Constants::PlayerWalkSpeed, m_animationAssets->walkingAnimation);
		m_idleRunBlender.AddPoint(Constants::PlayerSprintSpeed, m_animationAssets->runningAnimation);

		Nz::NodeComponent& node = entity.get<Nz::NodeComponent>();
		m_playerPosition = node.GetPosition();
	}

	void PlayerAnimationController::Animate(Nz::Skeleton& skeleton, Nz::Time elapsedTime)
	{
		m_idleRunBlender.UpdateAnimation(elapsedTime);
		m_idleRunBlender.AnimateSkeleton(&skeleton);
	}

	void PlayerAnimationController::UpdateStates(Nz::Time elapsedTime)
	{
		// Client doesn't know player speed as physics is not yet replicated from server
		Nz::NodeComponent& node = m_entity.get<Nz::NodeComponent>();
		Nz::Vector3f newPosition = node.GetPosition();
		Nz::Vector3f up = node.GetUp();

		float playerVelocity = Nz::Vector3f::Distance(newPosition.ProjectOnPlane(up), m_playerPosition.ProjectOnPlane(up)) / elapsedTime.AsSeconds();
		m_playerPosition = newPosition;

		m_playerVelocities.InsertValue(playerVelocity);
		m_idleRunBlender.UpdateValue(m_playerVelocities.GetAverageValue());
	}
}
