// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientSessionHandler.hpp>

namespace tsom
{
	template<typename F>
	void ClientSessionHandler::ForEachPlayer(F&& callback) const
	{
		for (PlayerIndex index = 0; index < m_players.size(); ++index)
		{
			auto& playerInfoOpt = m_players[index];
			if (playerInfoOpt)
				callback(index, *playerInfoOpt);
		}
	}

	inline entt::handle ClientSessionHandler::GetControlledEntity() const
	{
		return m_playerControlledEntity;
	}

	inline EntityRegistry& ClientSessionHandler::GetEntityRegistry()
	{
		return m_entityRegistry;
	}

	inline const EntityRegistry& ClientSessionHandler::GetEntityRegistry() const
	{
		return m_entityRegistry;
	}

	inline const GravityController* ClientSessionHandler::GetGravityController(Packets::Helper::EnvironmentId environmentIndex) const
	{
		if (environmentIndex >= m_environments.size() || !m_environments[environmentIndex])
			return nullptr;

		return m_environments[environmentIndex]->gravityController;
	}

	inline Packets::Helper::EnvironmentId ClientSessionHandler::GetRootEnvironmentId() const
	{
		return m_rootEnvironmentId;
	}

	inline Packets::Helper::EnvironmentId ClientSessionHandler::GetRootEnvironmentId(Packets::Helper::EnvironmentId environmentId) const
	{
		Packets::Helper::EnvironmentId rootEnvironmentId = environmentId;
		while (const EnvironmentData::ChildData* childData = std::get_if<EnvironmentData::ChildData>(&m_environments[rootEnvironmentId]->specData))
		{
			NazaraAssert(childData->parentEnvironment < m_environments.size() && m_environments[childData->parentEnvironment]);
			rootEnvironmentId = childData->parentEnvironment;
		}

		return rootEnvironmentId;
	}

	inline const Nz::Vector3i32& ClientSessionHandler::GetRootEnvironmentIndices(Packets::Helper::EnvironmentId rootEnvironmentId) const
	{
		NazaraAssert(rootEnvironmentId < m_environments.size() && m_environments[rootEnvironmentId]);
		NazaraAssertMsg(std::holds_alternative<EnvironmentData::RootData>(m_environments[rootEnvironmentId]->specData), "not a root environment");
		return std::get<EnvironmentData::RootData>(m_environments[rootEnvironmentId]->specData).indices;
	}

	inline ScriptingContext& ClientSessionHandler::GetScriptingContext()
	{
		return m_scriptingContext;
	}

	inline auto ClientSessionHandler::FetchPlayerInfo(PlayerIndex playerIndex) -> PlayerInfo*
	{
		if (playerIndex >= m_players.size() || !m_players[playerIndex])
			return nullptr;

		return &m_players[playerIndex].value();
	}

	inline auto ClientSessionHandler::FetchPlayerInfo(PlayerIndex playerIndex) const -> const PlayerInfo*
	{
		if (playerIndex >= m_players.size() || !m_players[playerIndex])
			return nullptr;

		return &m_players[playerIndex].value();
	}
}
