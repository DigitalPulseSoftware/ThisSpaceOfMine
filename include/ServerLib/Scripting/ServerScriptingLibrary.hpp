// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_SERVERLIB_SCRIPTING_SERVERSCRIPTINGLIBRARY_HPP
#define TSOM_SERVERLIB_SCRIPTING_SERVERSCRIPTINGLIBRARY_HPP

#include <ServerLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>
#include <memory>

namespace tsom
{
	class ServerEntityScriptingLibrary;
	class ServerInstance;

	class TSOM_SERVERLIB_API ServerScriptingLibrary : public ScriptingLibrary
	{
		public:
			inline ServerScriptingLibrary(ServerInstance& serverInstance, ServerEntityScriptingLibrary& entityScriptingLibrary);
			ServerScriptingLibrary(const ServerScriptingLibrary&) = delete;
			ServerScriptingLibrary(ServerScriptingLibrary&&) = delete;
			inline ~ServerScriptingLibrary();

			void Register(sol::state& state) override;

			ServerScriptingLibrary& operator=(const ServerScriptingLibrary&) = delete;
			ServerScriptingLibrary& operator=(ServerScriptingLibrary&&) = delete;

		private:
			void RegisterAtmosphere(sol::state& state);
			void RegisterEnvironment(sol::state& state);
			void RegisterPlayer(sol::state& state);
			void RegisterServer(sol::state& state);

			std::shared_ptr<bool> m_aliveSignal;
			ServerEntityScriptingLibrary& m_entityScriptingLibrary;
			ServerInstance& m_serverInstance;
	};
}

#include <ServerLib/Scripting/ServerScriptingLibrary.inl>

#endif // TSOM_SERVERLIB_SCRIPTING_SERVERSCRIPTINGLIBRARY_HPP
