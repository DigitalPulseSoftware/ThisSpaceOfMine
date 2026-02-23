// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_SHAREDSCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_SHAREDSCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>
#include <entt/entt.hpp>
#include <sol/state.hpp>

namespace Nz
{
	class Physics3DSystem;
}

namespace tsom
{
	class EntityRegistry;
	class SharedEntityScriptingLibrary;

	class TSOM_COMMONLIB_API SharedScriptingLibrary : public ScriptingLibrary
	{
		public:
			inline SharedScriptingLibrary(SharedEntityScriptingLibrary& entityScriptingLibrary);
			SharedScriptingLibrary(const SharedScriptingLibrary&) = delete;
			SharedScriptingLibrary(SharedScriptingLibrary&&) = delete;
			~SharedScriptingLibrary() = default;

			void Register(sol::state& state) override;

			SharedScriptingLibrary& operator=(const SharedScriptingLibrary&) = delete;
			SharedScriptingLibrary& operator=(SharedScriptingLibrary&&) = delete;

		private:
			void RegisterMovementControllers(sol::state& state);
			void RegisterPhysWorld(sol::state& state);

			SharedEntityScriptingLibrary& m_entityScriptingLibrary;
	};
}

#include <CommonLib/Scripting/SharedScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_SHAREDSCRIPTINGLIBRARY_HPP
