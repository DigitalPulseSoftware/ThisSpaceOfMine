// Copyright (C) 2024 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_SCRIPTINGPROPERTIES_HPP
#define TSOM_COMMONLIB_SCRIPTING_SCRIPTINGPROPERTIES_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/EntityProperties.hpp>
#include <sol/forward.hpp>

namespace tsom
{
	TSOM_COMMONLIB_API EntityProperty TranslatePropertyFromLua(sol::object value, EntityPropertyType expectedType, bool isArray);
	TSOM_COMMONLIB_API sol::object TranslatePropertyToLua(sol::state_view& lua, const EntityProperty& property);
}

#include <CommonLib/Scripting/ScriptingProperties.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_SCRIPTINGPROPERTIES_HPP
