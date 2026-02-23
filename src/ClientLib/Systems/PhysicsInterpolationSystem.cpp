// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/PhysicsInterpolationSystem.hpp>
#include <ClientLib/Components/PhysicsInterpolationComponent.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Components/RigidBody3DComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	void PhysicsInterpolationSystem::Update(Nz::Time /*elapsedTime*/)
	{
		auto& physWorld = m_physicsSystem.GetPhysWorld();
		Nz::Time timestepAccumulator = physWorld.GetTimestepAccumulator();
		Nz::Time stepSize = physWorld.GetStepSize();

		if (timestepAccumulator < m_prevAccumulator)
			m_prevAccumulator -= stepSize;

		float interpFactor = (timestepAccumulator - m_prevAccumulator).AsSeconds() / (stepSize - m_prevAccumulator).AsSeconds();
		m_prevAccumulator = timestepAccumulator;

		// Interpolation
		auto view = m_registry.view<Nz::NodeComponent, Nz::RigidBody3DComponent, PhysicsInterpolationComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, entityNode, rigidBody] : view.each())
		{
			Nz::Vector3f currentPos = entityNode.GetPosition();
			Nz::Quaternionf currentRot = entityNode.GetRotation();

			auto [targetPos, targetRot] = rigidBody.GetPositionAndRotation();

			Nz::Vector3f position = Nz::Vector3f::Lerp(currentPos, targetPos, interpFactor);
			Nz::Quaternionf rotation = Nz::Quaternionf::Slerp(currentRot, targetRot, interpFactor);

			entityNode.SetTransform(position, rotation);
		}

		auto referencedView = m_registry.view<Nz::NodeComponent, ReferencedPhysicsInterpolationComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [entity, entityNode, referenced] : referencedView.each())
		{
			Nz::Vector3f currentPos = entityNode.GetPosition();
			Nz::Quaternionf currentRot = entityNode.GetRotation();

			auto& targetNode = referenced.referenceEntity.get<Nz::NodeComponent>();
			Nz::Vector3f targetPos = targetNode.GetPosition();
			Nz::Quaternionf targetRot = targetNode.GetRotation();

			Nz::Vector3f position = Nz::Vector3f::Lerp(currentPos, targetPos, interpFactor);
			Nz::Quaternionf rotation = Nz::Quaternionf::Slerp(currentRot, targetRot, interpFactor);

			entityNode.SetTransform(position, rotation);
		}
	}
}
