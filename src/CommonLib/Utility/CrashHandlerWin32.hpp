// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_UTILITY_CRASHHANDLERWIN32_HPP
#define TSOM_COMMONLIB_UTILITY_CRASHHANDLERWIN32_HPP

// no include reordering

#include <CommonLib/Utility/CrashHandler.hpp>
#include <Nazara/Core/DynLib.hpp>
#include <array>
#include <string_view>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Dbghelp.h> //< Must be included after Windows.h

namespace tsom
{
	class CrashHandlerWin32 : public CrashHandler
	{
		public:
			CrashHandlerWin32() = default;
			~CrashHandlerWin32();

			bool Install() override;
			void HandleUnhandledException(const std::exception* e, const cpptrace::stacktrace& stacktrace) override;
			void Uninstall() override;

		private:
			void GenerateCrashdump(const wchar_t* filename, EXCEPTION_POINTERS* e);
			void GenerateCrashlog(const wchar_t* filename, std::string_view errorMessage, EXCEPTION_POINTERS* e, DWORD crashedThread, const cpptrace::stacktrace& stacktrace);

			static std::array<wchar_t, MAX_PATH> GetCrashdumpFilename();
			static std::string GetErrorMessage(EXCEPTION_RECORD* record);
			static LONG CALLBACK HandleException(EXCEPTION_POINTERS* e);

			Nz::DynLib m_windbg;
	};
}

#include <CommonLib/Utility/CrashHandlerWin32.inl>

#endif // TSOM_COMMONLIB_UTILITY_CRASHHANDLERWIN32_HPP
