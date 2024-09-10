// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_SCRIPTING_SCRIPTINGCONTEXT_HPP
#define TSOM_COMMONLIB_SCRIPTING_SCRIPTINGCONTEXT_HPP

#include <CommonLib/Export.hpp>
#include <NazaraUtils/Result.hpp>
#include <sol/sol.hpp>
#include <memory>
#include <vector>

namespace Nz
{
	class ApplicationBase;
}

namespace tsom
{
	class ScriptingLibrary;

	class TSOM_COMMONLIB_API ScriptingContext
	{
		public:
			using PrintCallback = std::function<void(std::string&& str)>;

			ScriptingContext(Nz::ApplicationBase& app);
			ScriptingContext(const ScriptingContext&) = delete;
			ScriptingContext(ScriptingContext&&) = delete;
			~ScriptingContext();

			Nz::Result<sol::object, std::string> Execute(std::string_view str, const std::string& origin);

			void LoadDirectory(std::string_view directoryPath);
			Nz::Result<sol::object, std::string> LoadFile(const std::string& filePath);

			PrintCallback OverridePrintCallback(PrintCallback&& printOutput);

			template<typename T, typename... Args> void RegisterLibrary(Args&&... args);
			void RegisterLibrary(std::unique_ptr<ScriptingLibrary>&& library);

			ScriptingContext& operator=(const ScriptingContext&) = delete;
			ScriptingContext& operator=(ScriptingContext&&) = delete;

		private:
			sol::state m_state;
			std::vector<std::unique_ptr<ScriptingLibrary>> m_libraries;
			PrintCallback m_printCallback;
			Nz::ApplicationBase& m_app;
	};
}

#include <CommonLib/Scripting/ScriptingContext.inl>

#endif // TSOM_COMMONLIB_SCRIPTING_SCRIPTINGCONTEXT_HPP
