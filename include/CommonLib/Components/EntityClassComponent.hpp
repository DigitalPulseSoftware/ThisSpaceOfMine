// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_ENTITYCLASSCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_ENTITYCLASSCOMPONENT_HPP

#include <CommonLib/EntityProperties.hpp>
#include <vector>

namespace tsom
{
	class EntityClass;

	struct EntityClassComponent
	{
		std::vector<EntityProperty> properties;
		const EntityClass* entityClass;
	};
}

#include <CommonLib/Components/EntityClassComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_ENTITYCLASSCOMPONENT_HPP
