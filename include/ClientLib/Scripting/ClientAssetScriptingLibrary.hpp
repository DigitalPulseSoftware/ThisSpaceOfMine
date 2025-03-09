// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SCRIPTING_CLIENTASSETSCRIPTINGLIBRARY_HPP
#define TSOM_CLIENTLIB_SCRIPTING_CLIENTASSETSCRIPTINGLIBRARY_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/Scripting/AssetScriptingLibrary.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class ClientSessionHandler;

	class TSOM_CLIENTLIB_API ClientAssetScriptingLibrary final : public AssetScriptingLibrary
	{
		public:
			using AssetScriptingLibrary::AssetScriptingLibrary;
			ClientAssetScriptingLibrary(const ClientAssetScriptingLibrary&) = delete;
			ClientAssetScriptingLibrary(ClientAssetScriptingLibrary&&) = delete;
			~ClientAssetScriptingLibrary() = default;

			void Register(sol::state& state) override;

			ClientAssetScriptingLibrary& operator=(const ClientAssetScriptingLibrary&) = delete;
			ClientAssetScriptingLibrary& operator=(ClientAssetScriptingLibrary&&) = delete;

		private:
			void RegisterAssetLibrary(sol::state& state);
			void RegisterMaterialInstance(sol::state& state);
			void RegisterRenderables(sol::state& state);
			void RegisterRenderStates(sol::state& state);
			void RegisterTexture(sol::state& state);
	};
}

#include <ClientLib/Scripting/ClientAssetScriptingLibrary.inl>

#endif // TSOM_CLIENTLIB_SCRIPTING_CLIENTASSETSCRIPTINGLIBRARY_HPP
