// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ServerInstance.hpp>
#include <CommonLib/Components/NetworkedComponent.hpp>
#include <CommonLib/Components/ServerPlayerControlledComponent.hpp>
#include <Nazara/JoltPhysics3D/Systems/JoltPhysics3DSystem.hpp>
#include <Nazara/Utility/Components/NodeComponent.hpp>

namespace tsom
{
	void ServerPlayer::Destroy()
	{
		m_instance.DestroyPlayer(m_playerIndex);
	}

	void ServerPlayer::HandleInputs(const PlayerInputs& inputs)
	{
		if (m_controller)
			m_controller->SetInputs(inputs);
	}

	void ServerPlayer::Respawn()
	{
		const Nz::Vector3f position = Nz::Vector3f::Up() * 45.f;
		const Nz::Quaternionf rotation = Nz::EulerAnglesf(-30.f, 0.f, 0.f);

		m_controlledEntity = m_instance.GetWorld().CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>();

		auto collider = std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);

		m_controller = std::make_shared<CharacterController>();
		m_controller->SetCurrentPlanet(&m_instance.GetPlanet());

		auto& physicsSystem = m_instance.GetWorld().GetSystem<Nz::JoltPhysics3DSystem>();

		auto& characterComponent = m_controlledEntity.emplace<Nz::JoltCharacterComponent>(physicsSystem.CreateCharacter(collider, position, rotation));
		characterComponent.SetImpl(m_controller);
		characterComponent.DisableSleeping();

		m_controlledEntity.emplace<ServerPlayerControlledComponent>(CreateHandle());
	}
}
