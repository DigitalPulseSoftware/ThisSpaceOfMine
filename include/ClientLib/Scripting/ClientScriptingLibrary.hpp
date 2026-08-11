// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SCRIPTING_CLIENTSCRIPTINGLIBRARY_HPP
#define TSOM_CLIENTLIB_SCRIPTING_CLIENTSCRIPTINGLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Scripting/SharedScriptingLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class ClientEntityScriptingLibrary;
	class ClientSessionHandler;
	class ConfigFile;

	class TSOM_CLIENTLIB_API ClientScriptingLibrary final : public SharedScriptingLibrary
	{
		public:
			ClientScriptingLibrary(Nz::ApplicationBase& app, ConfigFile& configFile, ClientSessionHandler& sessionHandler, ClientEntityScriptingLibrary& entityScriptingLibrary);
			ClientScriptingLibrary(const ClientScriptingLibrary&) = delete;
			ClientScriptingLibrary(ClientScriptingLibrary&&) = delete;
			~ClientScriptingLibrary() = default;

			void Register(sol::state& state) override;

			ClientScriptingLibrary& operator=(const ClientScriptingLibrary&) = delete;
			ClientScriptingLibrary& operator=(ClientScriptingLibrary&&) = delete;

		private:
			void RegisterClientSession(sol::state& state);
			void RegisterConfig(sol::state& state);
			void RegisterScripts(sol::state& state);

			Nz::ApplicationBase& m_app;
			ClientSessionHandler& m_sessionHandler;
			ConfigFile& m_config;
	};
}

#include <ClientLib/Scripting/ClientScriptingLibrary.inl>

#endif // TSOM_CLIENTLIB_SCRIPTING_CLIENTSCRIPTINGLIBRARY_HPP
