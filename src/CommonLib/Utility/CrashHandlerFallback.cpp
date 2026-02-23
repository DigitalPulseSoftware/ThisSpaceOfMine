// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Utility/CrashHandlerFallback.hpp>
#include <cpptrace/cpptrace.hpp>
#include <spdlog/spdlog.h>
#include <ctime>
#include <fstream>
#include <stdexcept>

namespace tsom
{
	bool CrashHandlerFallback::Install()
	{
		return false;
	}

	void CrashHandlerFallback::HandleUnhandledException(const std::exception* e, const cpptrace::stacktrace& stacktrace)
	{
		std::time_t currentTime = std::time(nullptr);
		std::tm* time = std::localtime(&currentTime);

		std::string filename = fmt::format("tsom_crashdump_{:04}{:02}{:02}_{:02}{:02}{:02}", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

		std::fstream dumpFile(filename, std::ios_base::out | std::ios_base::trunc);
		if (!dumpFile)
		{
			std::fprintf(stderr, "CrashDump: failed to create file %s\n", filename.c_str());
			return;
		}
		dumpFile << std::fixed;

		WriteHeader(dumpFile);
		if (e)
			dumpFile << "Unhandled C++ exception " << typeid(*e).name() << e->what();
		else
			dumpFile << "Unhandled unknown C++ exception";

		stacktrace.print(dumpFile, true);
	}

	void CrashHandlerFallback::Uninstall()
	{
	}
}
