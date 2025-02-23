// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerPlayer.hpp>
#include <CommonLib/CharacterController.hpp>
#include <CommonLib/ConsoleExecutor.hpp>
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
#include <ServerLib/Systems/NetworkedEntitiesSystem.hpp>
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
		environment->RegisterPlayer(this);

		if (m_visibilityHandler.CreateEnvironment(*environment, environmentOwner))
		{
			auto& networkedEntities = environment->GetWorld().GetSystem<NetworkedEntitiesSystem>();
			networkedEntities.CreateAllEntities(m_visibilityHandler);
		}
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
			m_console->scriptingContext.RegisterLibrary<MathScriptingLibrary>();
			m_console->scriptingContext.RegisterLibrary<ChunkScriptingLibrary>();
			ServerEntityScriptingLibrary& entityScriptingLibrary = m_console->scriptingContext.RegisterLibrary<ServerEntityScriptingLibrary>(m_serverInstance.GetEntityRegistry());
			m_console->scriptingContext.RegisterLibrary<SharedScriptingLibrary>(entityScriptingLibrary);
			m_console->scriptingContext.RegisterLibrary<ServerScriptingLibrary>(m_serverInstance, entityScriptingLibrary);

			sol::state& state = m_console->scriptingContext.GetState();
			state["CurrentPlayer"] = CreateHandle();

			m_console->executor.OnError.Connect([this](ConsoleExecutor* /*executor*/, std::string_view error)
			{
				Packets::ConsoleOutput consoleOutputPacket;
				consoleOutputPacket.color = Nz::Color::Red();
				consoleOutputPacket.output = std::string(error);

				GetSession()->SendPacket(std::move(consoleOutputPacket));
			});

			m_console->executor.OnOutput.Connect([this](ConsoleExecutor* /*executor*/, std::string_view error)
			{
				Packets::ConsoleOutput consoleOutputPacket;
				consoleOutputPacket.color = Nz::Color::White();
				consoleOutputPacket.output = std::string(error);

				GetSession()->SendPacket(std::move(consoleOutputPacket));
			});
		}

		m_console->executor.Execute(command, "remote client");
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
		if (m_inputBuffer.empty())
			return;

		// Downstream throttle jitter buffer (TODO: Convert to downstream throttle)
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
