// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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
#include <cassert>

namespace tsom
{
	ServerPlayer::~ServerPlayer()
	{
		if (m_controlledEntity)
			m_controlledEntity.destroy();

		if (m_environment)
			m_environment->UnregisterPlayer(this);
	}

	void ServerPlayer::Destroy()
	{
		m_instance.DestroyPlayer(m_playerIndex);
	}

	void ServerPlayer::PushInputs(const PlayerInputs& inputs)
	{
		m_inputQueue.push_back(inputs);
	}

	void ServerPlayer::Respawn(const Nz::Vector3f& position, const Nz::Quaternionf& rotation)
	{
		if (m_controlledEntity)
			m_controlledEntity.destroy();

		m_controlledEntity = m_environment->CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>();

		m_controller = std::make_shared<CharacterController>();
		m_controller->SetGravityController(m_environment->GetGravityController());

		m_visibilityHandler.UpdateControlledEntity(m_controlledEntity, m_controller.get()); // TODO: Reset to nullptr when player entity is destroyed

		Nz::PhysCharacter3DComponent::Settings characterSettings;
		characterSettings.collider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);
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
		assert(environment);

		if (m_environment)
			m_environment->UnregisterPlayer(this);

		m_environment = environment;
		m_environment->RegisterPlayer(this);

		if (!m_controlledEntity)
			return;

		Respawn(Nz::Vector3f::Zero(), Nz::Quaternionf::Identity());
	}

	void ServerPlayer::UpdateNickname(std::string nickname)
	{
		m_nickname = std::move(nickname);
	}
}
