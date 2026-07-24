// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ConsoleExecutor.hpp>
#include <CommonLib/ShipController.hpp>
#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/Scripting/AssetScriptingLibrary.hpp>
#include <CommonLib/Scripting/BaseScriptingLibrary.hpp>
#include <CommonLib/Scripting/ChunkScriptingLibrary.hpp>
#include <CommonLib/Scripting/MathScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingContext.hpp>
#include <CommonLib/Scripting/SharedScriptingLibrary.hpp>
#include <ServerLib/ServerInstance.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/ServerShipEnvironment.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
#include <ServerLib/Components/ServerPlayerControlledComponent.hpp>
#include <ServerLib/Scripting/ServerEntityScriptingLibrary.hpp>
#include <ServerLib/Scripting/ServerScriptingLibrary.hpp>
#include <ServerLib/Systems/EnvironmentProxySystem.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <cassert>

namespace tsom
{
	struct ServerPlayer::Console
	{
		Console(Nz::ApplicationBase& app) :
		scriptingContext(app),
		executor(scriptingContext)
		{
		}

		ScriptingContext scriptingContext;
		ConsoleExecutor executor;
	};

	ServerPlayer::ServerPlayer(ServerInstance& instance, PlayerIndex playerIndex, NetworkSession* session, const std::optional<Nz::Uuid>& uuid, std::string nickname, PlayerPermissionFlags permissions) :
	m_uuid(uuid),
	m_nickname(std::move(nickname)),
	m_console(nullptr),
	m_respawnTimer(Nz::Time::Zero()),
	m_inputQueueAdvancement(0),
	m_session(session),
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
			m_controlledEntity->destroy();

		for (ServerEnvironment* environment : m_registeredEnvironments)
			environment->UnregisterPlayer(this);
	}

	void ServerPlayer::AddToEnvironment(ServerEnvironment* environment, entt::handle environmentOwner)
	{
		assert(m_rootEnvironment);
		assert(!IsInEnvironment(environment));
		m_registeredEnvironments.push_back(environment);

		bool shouldCreateEntities = m_visibilityHandler.CreateEnvironment(*environment, environmentOwner);
		environment->RegisterPlayer(this, shouldCreateEntities);
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

	void ServerPlayer::Destroy()
	{
		m_serverInstance.DestroyPlayer(m_playerIndex);
	}

	void ServerPlayer::ExecuteConsoleCommand(std::string_view command)
	{
		if (!m_console)
		{
			Nz::ApplicationBase& applicationBase = m_serverInstance.GetApplication();

			m_console.Emplace(applicationBase);
			m_console->scriptingContext.RegisterLibrary<BaseScriptingLibrary>();
			m_console->scriptingContext.RegisterLibrary<AssetScriptingLibrary>(applicationBase);
			m_console->scriptingContext.RegisterLibrary<MathScriptingLibrary>();
			m_console->scriptingContext.RegisterLibrary<ChunkScriptingLibrary>();
			ServerEntityScriptingLibrary& entityScriptingLibrary = m_console->scriptingContext.RegisterLibrary<ServerEntityScriptingLibrary>(m_serverInstance.GetEntityRegistry());
			m_console->scriptingContext.RegisterLibrary<SharedScriptingLibrary>(entityScriptingLibrary);
			m_console->scriptingContext.RegisterLibrary<ServerScriptingLibrary>(m_serverInstance, entityScriptingLibrary);

			m_console->scriptingContext.LoadDirectory("scripts/libraries");

			sol::state& state = m_console->scriptingContext.GetState();
			state["CurrentPlayer"] = CreateHandle();

			m_console->executor.OnError.Connect([this](ConsoleExecutor* /*executor*/, std::string_view error)
			{
				Packets::S_ConsoleOutput consoleOutputPacket;
				consoleOutputPacket.color = Nz::Color::Red();
				consoleOutputPacket.output = std::string(error);

				GetSession()->SendPacket(std::move(consoleOutputPacket));
			});

			m_console->executor.OnOutput.Connect([this](ConsoleExecutor* /*executor*/, std::string_view error)
			{
				Packets::S_ConsoleOutput consoleOutputPacket;
				consoleOutputPacket.color = Nz::Color::White();
				consoleOutputPacket.output = std::string(error);

				GetSession()->SendPacket(std::move(consoleOutputPacket));
			});
		}

		m_console->executor.Execute(command, "remote client");
	}

	void ServerPlayer::ExitPiloting()
	{
		if (!m_controlledEntity)
			return;

		m_controller->SetShipController(nullptr);
		m_visibilityHandler.SetControlledShip({}, {}, Nz::Quaternionf::Identity());
	}

	ServerEnvironment* ServerPlayer::GetControlledEntityEnvironment()
	{
		if (!m_controlledEntity)
			return nullptr;

		return ServerEnvironment::GetEnvironment(m_controlledEntity);
	}

	const ServerEnvironment* ServerPlayer::GetControlledEntityEnvironment() const
	{
		if (!m_controlledEntity)
			return nullptr;

		return ServerEnvironment::GetEnvironment(m_controlledEntity);
	}

	EntityReference ServerPlayer::GetControlledShipEntityReference() const
	{
		if (!m_controlledEntity)
			return {};

		const auto& shipController = m_controller->GetShipController();
		if (!shipController)
			return {};

		return shipController->GetShipEntity();
	}

	void ServerPlayer::PilotShip(EntityReference shipEntity, EntityReference shipExteriorEntity, const Nz::Quaternionf& referenceRotation)
	{
		if (!m_controlledEntity)
			return;

		m_controller->SetShipController(std::make_shared<ShipController>(shipExteriorEntity, referenceRotation));
		m_visibilityHandler.SetControlledShip(shipEntity, shipExteriorEntity, referenceRotation);
	}

	void ServerPlayer::PushInputs(const PlayerInputs& inputs)
	{
		if (m_inputBuffer.size() == m_inputBuffer.capacity())
			m_inputBuffer.erase(m_inputBuffer.begin());

		m_inputBuffer.push_back(inputs);
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

		ExitPiloting();

		if (m_controlledEntity)
			m_controlledEntity->destroy();

		std::shared_ptr<const EntityClass> playerClass = m_serverInstance.GetEntityRegistry().FindClass("player");
		NazaraAssert(playerClass);

		entt::handle playerEntity = environment->CreateEntity();
		playerEntity.emplace<Nz::NodeComponent>(position, rotation);
		playerEntity.emplace<ClassInstanceComponent>(playerClass);
		playerEntity.emplace<NetworkedComponent>();
		playerEntity.emplace<ServerPlayerControlledComponent>(CreateHandle());

		m_controller = std::make_shared<CharacterController>();
		m_controller->SetGravityController(environment->GetGravityController());

		playerClass->InitAndActivateEntity(playerEntity);

		m_controlledEntity = playerEntity;
	}

	void ServerPlayer::SendChatMessage(std::string chatMessage)
	{
		Packets::S_ChatMessage chatMessagePacket;
		chatMessagePacket.message = std::move(chatMessage);

		GetSession()->SendPacket(std::move(chatMessagePacket));
	}

	void ServerPlayer::SetOwnedShip(std::unique_ptr<ServerShipEnvironment>&& ship)
	{
		m_ship = std::move(ship);
	}

	void ServerPlayer::Tick()
	{
		// Handle auto-respawn
		if (!m_controlledEntity)
		{
			if (m_respawnTimer > Nz::Time::Zero())
			{
				m_respawnTimer -= Constants::TickDuration;
				if (m_respawnTimer <= Nz::Time::Zero())
				{
					const auto& spawnpoint = m_serverInstance.GetDefaultSpawnpoint();
					Respawn(spawnpoint.env, spawnpoint.position, spawnpoint.rotation);
				}
			}
			else
				m_respawnTimer = Constants::PlayerRespawnTime;
		}

		if (m_inputBuffer.empty())
			return;

		// Downstream throttle jitter buffer (TODO: Convert to upstream throttle)
		// Adjust input consumption depending on how many inputs are sitting in the input queue
		Nz::UInt32 advancement = 1000;
		if (m_inputBuffer.size() > Constants::TargetInputBufferSize)
			advancement += std::min<std::size_t>((m_inputBuffer.size() - Constants::TargetInputBufferSize) * 100, 500);
		else if (m_inputBuffer.size() < Constants::TargetInputBufferSize)
			advancement -= std::min<std::size_t>((Constants::TargetInputBufferSize - m_inputBuffer.size()) * 100, 500);

		m_inputQueueAdvancement += advancement;
		if (m_inputQueueAdvancement >= 1000)
		{
			m_inputQueueAdvancement -= 1000;

			PlayerInputs inputs = m_inputBuffer.front();
			m_inputBuffer.erase(m_inputBuffer.begin());

			// Combine inputs
			while (!m_inputBuffer.empty() && m_inputQueueAdvancement >= 1000)
			{
				inputs.Merge(m_inputBuffer.front());
				m_inputBuffer.erase(m_inputBuffer.begin());
				m_inputQueueAdvancement -= 1000;
			}

			m_visibilityHandler.UpdateLastInputIndex(inputs.index);

			if (m_controller)
				m_controller->SetInputs(inputs);
		}
	}

	std::string ServerPlayer::ToString() const
	{
		return fmt::format("<Player #{}: {}>", m_playerIndex, m_nickname);
	}

	void ServerPlayer::UpdateRootEnvironment(ServerEnvironment* environment)
	{
		assert(environment);
		if (m_rootEnvironment == environment)
			return;

		ClearEnvironments();
		m_rootEnvironment = environment;

		AddToEnvironment(environment, entt::handle{});

		auto& envProxySystem = m_rootEnvironment->GetWorld().GetSystem<EnvironmentProxySystem>();
		envProxySystem.AddEnvironmentRecursively(this);
	}

	void ServerPlayer::UpdateNickname(std::string nickname)
	{
		m_nickname = std::move(nickname);
	}
}
