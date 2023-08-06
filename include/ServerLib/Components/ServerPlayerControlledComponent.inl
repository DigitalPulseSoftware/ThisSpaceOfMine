// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline ServerPlayerControlledComponent::ServerPlayerControlledComponent(ServerPlayerHandle playerHandle) :
	m_controllingPlayer(std::move(playerHandle))
	{
	}

	inline ServerPlayer* ServerPlayerControlledComponent::GetPlayer()
	{
		return m_controllingPlayer;
	}
}
