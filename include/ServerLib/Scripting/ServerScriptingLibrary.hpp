// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SCRIPTING_SERVERSCRIPTINGLIBRARY_HPP
#define TSOM_SERVERLIB_SCRIPTING_SERVERSCRIPTINGLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class TSOM_SERVERLIB_API ServerScriptingLibrary : public ScriptingLibrary
	{
		public:
			inline ServerScriptingLibrary(Nz::ApplicationBase& app);
			ServerScriptingLibrary(const ServerScriptingLibrary&) = delete;
			ServerScriptingLibrary(ServerScriptingLibrary&&) = delete;
			~ServerScriptingLibrary() = default;

			void Register(sol::state& state) override;

			ServerScriptingLibrary& operator=(const ServerScriptingLibrary&) = delete;
			ServerScriptingLibrary& operator=(ServerScriptingLibrary&&) = delete;

		private:
			Nz::ApplicationBase& m_app;
	};
}

#include <ServerLib/Scripting/ServerScriptingLibrary.inl>

#endif // TSOM_SERVERLIB_SCRIPTING_SERVERSCRIPTINGLIBRARY_HPP