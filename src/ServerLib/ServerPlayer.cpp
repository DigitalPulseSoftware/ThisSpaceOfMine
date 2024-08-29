// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/PhysicsConstants.hpp>
#include <CommonLib/Components/PlanetComponent.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <cassert>

namespace tsom
{
	ServerPlayer::ServerPlayer(ServerInstance& instance, PlayerIndex playerIndex, NetworkSession* session, const std::optional<Nz::Uuid>& uuid, std::string nickname, PlayerPermissionFlags permissions) :
	m_uuid(uuid),
	m_nickname(std::move(nickname)),
	m_session(session),
	m_controlledEntityEnvironment(nullptr),
	m_rootEnvironment(nullptr),
	m_visibilityHandler(m_session),
	m_serverInstance(instance),
	m_playerIndex(playerIndex),
	m_permissions(permissions)
	{
	}

	ServerPlayer::~ServerPlayer()
	{
		if (m_controlledEntity)
			m_controlledEntity.destroy();

		for (ServerEnvironment* environment : m_registeredEnvironments)
			environment->UnregisterPlayer(this);
	}

	void ServerPlayer::AddToEnvironment(ServerEnvironment* environment)
	{
		assert(m_rootEnvironment);
		assert(!IsInEnvironment(environment));

		EnvironmentTransform transform;
		if (!m_rootEnvironment->GetEnvironmentTransformation(*environment, &transform))
			assert(false && "environment is not linked to root environment");

		HandleNewEnvironment(environment, transform);
	}

	void ServerPlayer::Destroy()
	{
		m_serverInstance.DestroyPlayer(m_playerIndex);
	}

	void ServerPlayer::MoveEntityToEnvironment(ServerEnvironment* environment)
	{
		assert(IsInEnvironment(environment));

		if (m_controlledEntityEnvironment == environment)
			return;

		if (!m_controlledEntity)
			return;

		entt::handle previousEntity = m_controlledEntity;
		Nz::NodeComponent& previousNode = previousEntity.get<Nz::NodeComponent>();
		Nz::Vector3f position = previousNode.GetPosition();
		Nz::Quaternionf rotation = previousNode.GetRotation();
		Nz::Vector3f up = previousNode.GetUp();

		auto& previousCharacter = previousEntity.get<Nz::PhysCharacter3DComponent>();
		auto [linearVel, angularVel] = previousCharacter.GetLinearAndAngularVelocity();

		auto& networkedSystem = m_controlledEntityEnvironment->GetWorld().GetSystem<NetworkedEntitiesSystem>();
		networkedSystem.ForgetEntity(previousEntity);

		EnvironmentTransform prevToNewTransform;
		if (!environment->GetEnvironmentTransformation(*m_controlledEntityEnvironment, &prevToNewTransform))
			assert(false && "old environment is not linked to the new");

		Nz::Vector3f prevEnvironmentUp = -m_controlledEntityEnvironment->GetGravityController()->ComputeGravity(position).direction;
		Nz::Quaternionf environmentRotationCorrection = Nz::Quaternionf::RotationBetween(prevEnvironmentUp, Nz::Vector3f::Up());

		position = prevToNewTransform.Translate(position);
		rotation = prevToNewTransform.Rotate(rotation);
		linearVel = prevToNewTransform.Rotate(linearVel);
		angularVel = prevToNewTransform.Rotate(angularVel);

		m_controlledEntity = environment->CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>(false); //< Don't create entity

		m_controller->SetGravityController(environment->GetGravityController());
		m_controller->RotateInstantaneously(prevToNewTransform.rotation);

		Nz::PhysCharacter3DComponent::Settings characterSettings;
		characterSettings.collider = previousCharacter.GetCollider();
		characterSettings.position = position;
		characterSettings.rotation = rotation;
		characterSettings.objectLayer = previousCharacter.GetObjectLayer();

		auto& characterComponent = m_controlledEntity.emplace<Nz::PhysCharacter3DComponent>(std::move(characterSettings));
		characterComponent.SetImpl(m_controller);
		characterComponent.SetLinearAndAngularVelocity(linearVel, angularVel);
		characterComponent.SetUp(rotation * environmentRotationCorrection * up);
		characterComponent.DisableSleeping();

		// Force controller update to ensure new position will be sent
		m_controller->UpdatePosition(characterComponent);

		m_controlledEntity.emplace<ServerPlayerControlledComponent>(CreateHandle());

		m_controlledEntityEnvironment->ForEachPlayer([&](ServerPlayer& serverPlayer)
		{
			auto& visibilityHandler = serverPlayer.GetVisibilityHandler();
			visibilityHandler.UpdateEntityEnvironment(*environment, previousEntity, m_controlledEntity);
		});

		// Destroy previous entity before updating controlled entity, as entity destruction will not be forwarded to visibility handler
		// we need it to not add back entity to its moving entity list (FIXME: maybe the visibility handler should be the only one to handle that)
		previousEntity.destroy();

		m_visibilityHandler.UpdateControlledEntity(m_controlledEntity, m_controller.get()); // TODO: Reset to nullptr when player entity is destroyed

		m_controlledEntityEnvironment = environment;
	}

	void ServerPlayer::PushInputs(const PlayerInputs& inputs)
	{
		m_inputQueue.push_back(inputs);
	}

	void ServerPlayer::RemoveFromEnvironment(ServerEnvironment* environment)
	{
		assert(IsInEnvironment(environment));
		auto it = std::find(m_registeredEnvironments.begin(), m_registeredEnvironments.end(), environment);
		assert(it != m_registeredEnvironments.end());
		m_registeredEnvironments.erase(it);
		environment->UnregisterPlayer(this);

		m_visibilityHandler.DestroyEnvironment(*environment);
	}

	void ServerPlayer::Respawn(ServerEnvironment* environment, const Nz::Vector3f& position, const Nz::Quaternionf& rotation)
	{
		assert(IsInEnvironment(environment));

		if (m_controlledEntity)
			m_controlledEntity.destroy();

		m_controlledEntityEnvironment = environment;

		m_controlledEntity = environment->CreateEntity();
		m_controlledEntity.emplace<Nz::NodeComponent>(position, rotation);
		m_controlledEntity.emplace<NetworkedComponent>();

		m_controller = std::make_shared<CharacterController>();
		m_controller->SetGravityController(environment->GetGravityController());

		m_visibilityHandler.UpdateControlledEntity(m_controlledEntity, m_controller.get()); // TODO: Reset to nullptr when player entity is destroyed

		Nz::PhysCharacter3DComponent::Settings characterSettings;
		characterSettings.collider = std::make_shared<Nz::CapsuleCollider3D>(Constants::PlayerCapsuleHeight, Constants::PlayerColliderRadius);
		characterSettings.position = position;
		characterSettings.rotation = rotation;
		characterSettings.objectLayer = Constants::ObjectLayerPlayer;

		auto& characterComponent = m_controlledEntity.emplace<Nz::PhysCharacter3DComponent>(std::move(characterSettings));
		characterComponent.SetImpl(m_controller);
		characterComponent.DisableSleeping();

		m_controlledEntity.emplace<ServerPlayerControlledComponent>(CreateHandle());
	}

	void ServerPlayer::SendChatMessage(std::string chatMessage)
	{
		Packets::ChatMessage chatMessagePacket;
		chatMessagePacket.message = std::move(chatMessage);

		GetSession()->SendPacket(std::move(chatMessagePacket));
	}

	void ServerPlayer::SetOwnedShip(std::unique_ptr<ServerShipEnvironment>&& ship)
	{
		m_ship = std::move(ship);
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

	void ServerPlayer::UpdateRootEnvironment(ServerEnvironment* environment)
	{
		assert(environment);

		EnvironmentTransform oldToNewEnv(Nz::Vector3f::Zero(), Nz::Quaternionf::Identity());
		if (m_rootEnvironment)
		{
			m_rootEnvironment->GetEnvironmentTransformation(*environment, &oldToNewEnv);
			ClearEnvironments();
		}

		m_rootEnvironment = environment;
		HandleNewEnvironment(m_rootEnvironment, oldToNewEnv);

		m_rootEnvironment->ForEachConnectedEnvironment([&](ServerEnvironment& connectedEnvironment, const EnvironmentTransform& transform)
		{
			// avoid cycles
			EnvironmentTransform globalTransform = oldToNewEnv + transform;
			HandleNewEnvironment(&connectedEnvironment, oldToNewEnv + transform);
		});

		m_visibilityHandler.UpdateRootEnvironment(*m_rootEnvironment);
	}

	void ServerPlayer::ClearEnvironments()
	{
		for (ServerEnvironment* environment : m_registeredEnvironments)
		{
			environment->UnregisterPlayer(this);
			m_visibilityHandler.DestroyEnvironment(*environment);
		}
		m_registeredEnvironments.clear();
	}

	void ServerPlayer::HandleNewEnvironment(ServerEnvironment* environment, const EnvironmentTransform& transform)
	{
		assert(!IsInEnvironment(environment));
		m_registeredEnvironments.push_back(environment);
		environment->RegisterPlayer(this);

		if (m_visibilityHandler.CreateEnvironment(*environment, transform))
		{
			auto& networkedEntities = environment->GetWorld().GetSystem<NetworkedEntitiesSystem>();
			networkedEntities.CreateAllEntities(m_visibilityHandler);
		}
	}

	void ServerPlayer::UpdateNickname(std::string nickname)
	{
		m_nickname = std::move(nickname);
	}
}
