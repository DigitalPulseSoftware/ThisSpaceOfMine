// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utility/CrashHandler.hpp>
#include <CommonLib/Utils.hpp>
#include <CommonLib/Version.hpp>
#include <Nazara/Core/HardwareInfo.hpp>
#include <NazaraUtils/Prerequisites.hpp>
#include <ostream>

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

	void CrashHandler::WriteHeader(std::ostream& filestream)
	{
		filestream << std::fixed;

		filestream << "Game version: " << GetVersionInfo() << '\n';
		filestream << "Build info: " << GetBuildInfo() << '\n';

		Nz::HardwareInfo hardwareInfo;
		filestream << "CPU: " << hardwareInfo.GetCpuBrandString() << " (" << hardwareInfo.GetCpuThreadCount() << " threads)\n";
		filestream << "CPU Vendor: " << hardwareInfo.GetCpuVendorName() << '\n';
		filestream << "System memory: " << ByteToString(hardwareInfo.GetSystemTotalMemory()) << '\n';
		filestream << '\n';
	}
}
