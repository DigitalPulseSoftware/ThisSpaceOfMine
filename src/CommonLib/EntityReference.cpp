// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/EntityReference.hpp>

namespace tsom
{
	EntityReference::EntityReference(entt::handle entity)
	{
		if (HandleOwner* handleOwnerPtr = entity.try_get<HandleOwner>())
			m_handleData = handleOwnerPtr->handleData;
		else
		{
			auto& handleOwner = entity.emplace<HandleOwner>();
			handleOwner.handleData = std::make_shared<HandleData>();
			handleOwner.handleData->entity = entity;

			m_handleData = handleOwner.handleData;
		}
	}

	std::shared_ptr<EntityReference::HandleData> EntityReference::GetEmptyHandle()
	{
		static std::shared_ptr<HandleData> emptyHandle = std::make_shared<HandleData>();
		return emptyHandle;
	}
}
