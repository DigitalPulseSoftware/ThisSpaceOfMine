// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ServerWorld.hpp>
#include <CommonLib/Components/NetworkedComponent.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>

namespace tsom
{
	void ServerPlayer::Respawn()
	{
		const Nz::Vector3f position = Nz::Vector3f::Up() * 3.f + Nz::Vector3f::Backward() * 2.f;
		const Nz::Quaternionf rotation = Nz::EulerAnglesf(-30.f, 0.f, 0.f);

		m_controlledEntity = m_world.GetWorld().CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>();

		auto collider = std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);

		auto controller = std::make_shared<CharacterController>();
		controller->SetCurrentPlanet(&m_world.GetPlanet());

		auto& physicsSystem = m_world.GetWorld().GetSystem<Nz::JoltPhysics3DSystem>();

		auto& characterComponent = m_controlledEntity.emplace<Nz::JoltCharacterComponent>(physicsSystem.CreateCharacter(collider, position, rotation));
		characterComponent.SetImpl(controller);
		characterComponent.DisableSleeping();
	}
}
