// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_BASESCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_BASESCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>

namespace tsom
{
	class TSOM_COMMONLIB_API BaseScriptingLibrary : public ScriptingLibrary
	{
		public:
			BaseScriptingLibrary() = default;
			BaseScriptingLibrary(const BaseScriptingLibrary&) = delete;
			BaseScriptingLibrary(BaseScriptingLibrary&&) = delete;
			~BaseScriptingLibrary() = default;

			void Register(sol::state& state) override;

			BaseScriptingLibrary& operator=(const BaseScriptingLibrary&) = delete;
			BaseScriptingLibrary& operator=(BaseScriptingLibrary&&) = delete;

		private:
			void RegisterTime(sol::state& state);
	};
}

#include <CommonLib/Scripting/BaseScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_BASESCRIPTINGLIBRARY_HPP
