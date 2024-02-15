// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utility/CrashHandler.hpp>
#include <NazaraUtils/Prerequisites.hpp>

#ifdef NAZARA_PLATFORM_WINDOWS
#include <CommonLib/Utility/CrashHandlerWin32.hpp>
#else
#include <CommonLib/Utility/CrashHandlerFallback.hpp>
#endif

namespace tsom
{
	CrashHandler::~CrashHandler() = default;

	std::unique_ptr<CrashHandler> CrashHandler::PlatformCrashHandler()
	{
#ifdef NAZARA_PLATFORM_WINDOWS
		return std::make_unique<CrashHandlerWin32>();
#else
		return std::make_unique<CrashHandlerFallback>();
#endif
	}
}
