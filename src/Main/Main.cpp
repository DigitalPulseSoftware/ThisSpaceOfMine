// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Main/Main.hpp>
#include <CommonLib/Version.hpp>
#include <CommonLib/Utility/CrashHandler.hpp>
#include <cpptrace/from_current.hpp>
#include <spdlog/spdlog.h>
#include <cstdio>
#include <exception>

#ifdef NAZARA_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

int TSOMEntry(int argc, char* argv[], int(*mainFunc)(int argc, char* argv[]))
{
	// Enable line-buffering for stdout
	std::size_t buffSize = BUFSIZ;
#ifdef NAZARA_PLATFORM_POSIX
	// try to get system preferred stdout blocksize
	if (struct stat stat; fstat(STDOUT_FILENO, &stat) == 0)
		buffSize = stat.st_blksize;
	else
		spdlog::warn("failed to fstat stdout ({})", errno);
#endif

	std::setvbuf(stdout, nullptr, _IOLBF, buffSize);

	spdlog::info("TSOM {0}.{1}.{2} {3} ({4}) - {5}", tsom::GameMajorVersion, tsom::GameMinorVersion, tsom::GamePatchVersion, tsom::BuildBranch, tsom::BuildCommit, tsom::BuildCommitDate);

#ifdef NAZARA_PLATFORM_WINDOWS
	if (::IsDebuggerPresent())
		return mainFunc(argc, argv);
#endif

	std::unique_ptr<tsom::CrashHandler> crashHandler = tsom::CrashHandler::PlatformCrashHandler();
	crashHandler->Install();

	int retCode = EXIT_FAILURE;
	cpptrace::try_catch(
	[&]
	{
		retCode = mainFunc(argc, argv);
	},
	[&](const std::exception& e)
	{
		spdlog::critical("unhandled exception: {0}", e.what());
		crashHandler->HandleUnhandledException(&e, cpptrace::from_current_exception());
	},
	[&](/*...*/)
	{
		spdlog::critical("unhandled non-standard exception");
		crashHandler->HandleUnhandledException(nullptr, cpptrace::from_current_exception());
	});

	return retCode;
}

//TODO: Handle WinMain
