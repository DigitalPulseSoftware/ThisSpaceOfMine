// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/ServerWorldAppComponent.hpp>

namespace tsom
{
	void ServerWorldAppComponent::Update(Nz::Time elapsedTime)
	{
		for (auto& worldPtr : m_worlds)
			worldPtr->Update(elapsedTime);
	}
}
