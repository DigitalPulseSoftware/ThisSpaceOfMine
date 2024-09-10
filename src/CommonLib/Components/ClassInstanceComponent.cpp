// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Components/ClassInstanceComponent.hpp>
#include <CommonLib/EntityClass.hpp>
#include <fmt/format.h>

namespace tsom
{
	Nz::UInt32 ClassInstanceComponent::GetPropertyIndex(std::string_view propertyName) const
	{
		Nz::UInt32 propertyIndex = entityClass->FindProperty(propertyName);
		if (propertyIndex == EntityClass::InvalidIndex)
			throw std::runtime_error(fmt::format("invalid property {}", propertyName));

		return propertyIndex;
	}
}
