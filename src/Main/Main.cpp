// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Main/Main.hpp>
#include <CommonLib/Version.hpp>
#include <CommonLib/Utility/CrashHandler.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
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
		fmt::print(fg(fmt::color::yellow), "failed to fstat stdout ({})", errno);
#endif

	std::setvbuf(stdout, nullptr, _IOLBF, buffSize);

	fmt::print(fg(fmt::color::white), "TSOM {0}.{1}.{2} {3} ({4}) - {5}\n", tsom::GameMajorVersion, tsom::GameMinorVersion, tsom::GamePatchVersion, tsom::BuildBranch, tsom::BuildCommit, tsom::BuildCommitDate);

#ifdef NAZARA_PLATFORM_WINDOWS
	if (::IsDebuggerPresent())
		return mainFunc(argc, argv);
#endif

	std::unique_ptr<tsom::CrashHandler> crashHandler = tsom::CrashHandler::PlatformCrashHandler();
	crashHandler->Install();

	try
	{
		return mainFunc(argc, argv);
	}
	catch (const std::exception& e)
	{
		fmt::print(stderr, "unhandled exception: {0}\n", e.what());
		throw;
	}
	catch (...)
	{
		fmt::print(stderr, "unhandled non-standard exception\n");
		throw;
	}
}

//TODO: Handle WinMain
