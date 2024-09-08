// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_SCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_SCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <sol/forward.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API ScriptingLibrary
	{
		public:
			ScriptingLibrary() = default;
			ScriptingLibrary(const ScriptingLibrary&) = delete;
			ScriptingLibrary(ScriptingLibrary&&) = delete;
			virtual ~ScriptingLibrary();

			virtual void Register(sol::state& state) = 0;

			ScriptingLibrary& operator=(const ScriptingLibrary&) = delete;
			ScriptingLibrary& operator=(ScriptingLibrary&&) = delete;
	};
}

#include <CommonLib/Scripting/ScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_SCRIPTINGLIBRARY_HPP
