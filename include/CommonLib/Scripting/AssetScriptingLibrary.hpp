// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_ASSETSCRIPTINGLIBRARY_HPP
#define TSOM_COMMONLIB_SCRIPTING_ASSETSCRIPTINGLIBRARY_HPP

#include <CommonLib/Export.hpp>
#include <CommonLib/Scripting/ScriptingLibrary.hpp>
#include <entt/entt.hpp>
#include <sol/state.hpp>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class EntityRegistry;

	class TSOM_COMMONLIB_API AssetScriptingLibrary : public ScriptingLibrary
	{
		public:
			inline AssetScriptingLibrary(Nz::ApplicationBase& app);
			AssetScriptingLibrary(const AssetScriptingLibrary&) = delete;
			AssetScriptingLibrary(AssetScriptingLibrary&&) = delete;
			~AssetScriptingLibrary() = default;

			void Register(sol::state& state) override;

			AssetScriptingLibrary& operator=(const AssetScriptingLibrary&) = delete;
			AssetScriptingLibrary& operator=(AssetScriptingLibrary&&) = delete;

		protected:
			Nz::ApplicationBase& m_app;

		private:
			void RegisterMesh(sol::state& state);
			void RegisterPrimitive(sol::state& state);
			void RegisterSubMesh(sol::state& state);
	};
}

#include <CommonLib/Scripting/AssetScriptingLibrary.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_ASSETSCRIPTINGLIBRARY_HPP
