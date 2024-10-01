// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_SHAREDSCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_SHAREDSCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>
#include <entt/entt.hpp>
#include <sol/state.hpp>

namespace tsom
{
	class EntityRegistry;

	class TSOM_COMMONLIB_API SharedScriptingLibrary : public ScriptingLibrary
	{
		public:
			SharedScriptingLibrary() = default;
			SharedScriptingLibrary(const SharedScriptingLibrary&) = delete;
			SharedScriptingLibrary(SharedScriptingLibrary&&) = delete;
			~SharedScriptingLibrary() = default;

			void Register(sol::state& state) override;

			SharedScriptingLibrary& operator=(const SharedScriptingLibrary&) = delete;
			SharedScriptingLibrary& operator=(SharedScriptingLibrary&&) = delete;

		private:
			void RegisterMovementControllers(sol::state& state);
	};
}

#include <CommonLib/Scripting/SharedScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_SHAREDSCRIPTINGLIBRARY_HPP
