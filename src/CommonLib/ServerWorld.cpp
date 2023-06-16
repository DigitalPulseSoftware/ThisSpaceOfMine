// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ServerWorld.hpp>

namespace tsom
{
	void ServerWorld::Update(Nz::Time elapsedTime)
	{
		for (auto&& sessionManagerPtr : m_sessionManagers)
			sessionManagerPtr->Poll();
	}
}
