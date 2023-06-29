// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <CommonLib/ServerPlayer.hpp>
#include <CommonLib/ServerWorld.hpp>

namespace tsom
{
	void ServerPlayer::Respawn()
	{
		m_controlledEntity = m_world.GetWorld().CreateEntity();
	}
}
