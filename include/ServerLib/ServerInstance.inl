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

	inline Nz::Time ServerInstance::GetTickDuration() const
	{
		return m_tickDuration;
	}
}
