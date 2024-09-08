// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	template<typename... Args>
	NetworkSessionManager& ServerInstance::AddSessionManager(Args&& ...args)
	{
		return *m_sessionManagers.emplace_back(std::make_unique<NetworkSessionManager>(std::forward<Args>(args)...));
	}

	inline ServerPlayer* ServerInstance::FindPlayerByNickname(std::string_view nickname)
	{
		for (ServerPlayer& serverPlayer : m_players)
		{
			if (serverPlayer.GetNickname() == nickname)
				return &serverPlayer;
		}

		return nullptr;
	}

	inline const ServerPlayer* ServerInstance::FindPlayerByNickname(std::string_view nickname) const
	{
		for (const ServerPlayer& serverPlayer : m_players)
		{
			if (serverPlayer.GetNickname() == nickname)
				return &serverPlayer;
		}

		return nullptr;
	}

	inline ServerPlayer* ServerInstance::FindPlayerByUuid(const Nz::Uuid& uuid)
	{
		for (ServerPlayer& serverPlayer : m_players)
		{
			if (serverPlayer.GetUuid() == uuid)
				return &serverPlayer;
		}

		return nullptr;
	}

	inline const ServerPlayer* ServerInstance::FindPlayerByUuid(const Nz::Uuid& uuid) const
	{
		for (const ServerPlayer& serverPlayer : m_players)
		{
			if (serverPlayer.GetUuid() == uuid)
				return &serverPlayer;
		}

		return nullptr;
	}

	template<typename F> void ServerInstance::ForEachPlayer(F&& functor)
	{
		for (ServerPlayer& serverPlayer : m_players)
			functor(serverPlayer);
	}

	template<typename F> void ServerInstance::ForEachPlayer(F&& functor) const
	{
		for (const ServerPlayer& serverPlayer : m_players)
			functor(serverPlayer);
	}

	inline Nz::ApplicationBase& ServerInstance::GetApplication()
	{
		return m_application;
	}

	inline const BlockLibrary& ServerInstance::GetBlockLibrary() const
	{
		return m_blockLibrary;
	}

	inline const std::array<std::uint8_t, 32>& ServerInstance::GetConnectionTokenEncryptionKey() const
	{
		return m_connectionTokenEncryptionKey;
	}

	inline auto ServerInstance::GetDefaultSpawnpoint() const -> const Spawnpoint&
	{
		return m_defaultSpawnpoint;
	}

	inline EntityRegistry& ServerInstance::GetEntityRegistry()
	{
		return m_entityRegistry;
	}

	inline const EntityRegistry& ServerInstance::GetEntityRegistry() const
	{
		return m_entityRegistry;
	}

	inline ServerPlayer* ServerInstance::GetPlayer(PlayerIndex playerIndex)
	{
		return m_players.RetrieveFromIndex(playerIndex);
	}

	inline const ServerPlayer* ServerInstance::GetPlayer(PlayerIndex playerIndex) const
	{
		return m_players.RetrieveFromIndex(playerIndex);
	}

	inline Nz::Time ServerInstance::GetTickDuration() const
	{
		return m_tickDuration;
	}

	inline void ServerInstance::SetDefaultSpawnpoint(ServerEnvironment* environment, Nz::Vector3f position, Nz::Quaternionf rotation)
	{
		m_defaultSpawnpoint = Spawnpoint{ environment, rotation, position };
	}
}
