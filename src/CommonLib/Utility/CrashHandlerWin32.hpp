// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_COMMONLIB_CRASHHANDLER_WIN32_HPP
#define TSOM_COMMONLIB_CRASHHANDLER_WIN32_HPP

#include <CommonLib/Utility/CrashHandler.hpp>
#include <Nazara/Core/DynLib.hpp>
#include <windows.h>
#include <Dbghelp.h> //< Must be included after windows.h

namespace tsom
{
	class CrashHandlerWin32 : public CrashHandler
	{
		public:
			CrashHandlerWin32() = default;
			~CrashHandlerWin32();

			bool Install() override;
			void Uninstall() override;

		private:
			Nz::DynLib m_windbg;
	};
}

#include <CommonLib/Utility/CrashHandlerWin32.inl>

#endif
