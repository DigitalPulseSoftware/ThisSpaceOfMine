// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ServerLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/Components/PlanetGravityComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
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
		constexpr Nz::Vector3f position = Nz::Vector3f::Up() * 165.f;
		const Nz::Quaternionf rotation = Nz::EulerAnglesf(-30.f, 0.f, 0.f);

		m_controlledEntity = m_instance.GetWorld().CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>();
		auto& planetGravity = m_controlledEntity.emplace<PlanetGravityComponent>();
		planetGravity.planet = &m_instance.GetPlanet();

		auto collider = std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);

		m_controller = std::make_shared<CharacterController>();
		m_controller->SetCurrentPlanet(&m_instance.GetPlanet());

		auto& physicsSystem = m_instance.GetWorld().GetSystem<Nz::JoltPhysics3DSystem>();

		Nz::JoltCharacterComponent::Settings characterSettings;
		characterSettings.collider = std::make_shared<Nz::JoltCapsuleCollider3D>(1.8f, 0.4f);
		characterSettings.position = position;
		characterSettings.rotation = rotation;

		auto& characterComponent = m_controlledEntity.emplace<Nz::JoltCharacterComponent>(std::move(characterSettings));
		characterComponent.SetImpl(m_controller);
		characterComponent.DisableSleeping();

		m_controlledEntity.emplace<ServerPlayerControlledComponent>(CreateHandle());
	}
}
