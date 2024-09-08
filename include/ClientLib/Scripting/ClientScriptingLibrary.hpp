// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SCRIPTING_CLIENTSCRIPTINGLIBRARY_HPP
#define TSOM_CLIENTLIB_SCRIPTING_CLIENTSCRIPTINGLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ClientScriptingLibrary : public ScriptingLibrary
	{
		public:
			inline ClientScriptingLibrary(Nz::ApplicationBase& app);
			ClientScriptingLibrary(const ClientScriptingLibrary&) = delete;
			ClientScriptingLibrary(ClientScriptingLibrary&&) = delete;
			~ClientScriptingLibrary() = default;

			void Register(sol::state& state) override;

			ClientScriptingLibrary& operator=(const ClientScriptingLibrary&) = delete;
			ClientScriptingLibrary& operator=(ClientScriptingLibrary&&) = delete;

		private:
			void RegisterAssetLibrary(sol::state& state);
			void RegisterMaterialInstance(sol::state& state);
			void RegisterRenderables(sol::state& state);
			void RegisterTexture(sol::state& state);

			Nz::ApplicationBase& m_app;
	};
}

#include <ClientLib/Scripting/ClientScriptingLibrary.inl>

#endif // TSOM_CLIENTLIB_SCRIPTING_CLIENTSCRIPTINGLIBRARY_HPP
