// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>

namespace tsom
{
	void ServerPlayer::Destroy()
	{
		m_instance.DestroyPlayer(m_playerIndex);
	}

	void ServerPlayer::PushInputs(const PlayerInputs& inputs)
	{
		m_inputQueue.push_back(inputs);
	}

	void ServerPlayer::Respawn()
	{
		constexpr Nz::Vector3f position = Nz::Vector3f::Up() * 100.f + Nz::Vector3f::Backward() * 5.f;
		const Nz::Quaternionf rotation = Nz::EulerAnglesf(0.f, 0.f, 0.f);

		ServerPlanetEnvironment* serverPlanet = Nz::SafeCast<ServerPlanetEnvironment*>(m_environment);

		if (m_controlledEntity)
			m_controlledEntity.destroy();

		m_controlledEntity = serverPlanet->GetWorld().CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>();
		auto& planetGravity = m_controlledEntity.emplace<PlanetComponent>();
		planetGravity.planet = &serverPlanet->GetPlanet();

		auto collider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);

		m_controller = std::make_shared<CharacterController>();
		m_controller->SetCurrentPlanet(&serverPlanet->GetPlanet());
		m_visibilityHandler.UpdateControlledEntity(m_controlledEntity, m_controller.get()); // TODO: Reset to nullptr when player entity is destroyed

		auto& physicsSystem = serverPlanet->GetWorld().GetSystem<Nz::Physics3DSystem>();

		Nz::PhysCharacter3DComponent::Settings characterSettings;
		characterSettings.collider = collider;
		characterSettings.position = position;
		characterSettings.rotation = rotation;

		auto& characterComponent = m_controlledEntity.emplace<Nz::PhysCharacter3DComponent>(std::move(characterSettings));
		characterComponent.SetImpl(m_controller);
		characterComponent.DisableSleeping();

		m_controlledEntity.emplace<ServerPlayerControlledComponent>(CreateHandle());
	}

	void ServerPlayer::Tick()
	{
		if (!m_inputQueue.empty())
		{
			const PlayerInputs& inputs = m_inputQueue.front();

			m_visibilityHandler.UpdateLastInputIndex(inputs.index);

			if (m_controller)
				m_controller->SetInputs(inputs);

			m_inputQueue.erase(m_inputQueue.begin());
		}
	}

	void ServerPlayer::UpdateEnvironment(ServerEnvironment* environment)
	{
		m_environment = environment;
	}
}
