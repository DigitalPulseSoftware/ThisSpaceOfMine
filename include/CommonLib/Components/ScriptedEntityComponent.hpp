// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_COMPONENTS_SCRIPTEDENTITYCOMPONENT_HPP
#define TSOM_COMMONLIB_COMPONENTS_SCRIPTEDENTITYCOMPONENT_HPP

#include <CommonLib/Export.hpp>
#include <sol/table.hpp>

namespace tsom
{
	struct ScriptedEntityComponent
	{
		sol::table classMetatable;
		sol::table entityTable;
	};
}

#include <CommonLib/Components/ScriptedEntityComponent.inl>

#endif // TSOM_COMMONLIB_COMPONENTS_SCRIPTEDENTITYCOMPONENT_HPP
