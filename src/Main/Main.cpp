// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Main/Main.hpp>
#include <CommonLib/Version.hpp>
#include <CommonLib/Utility/CrashHandler.hpp>
#include <fmt/format.h>
#include <exception>

#ifdef NAZARA_PLATFORM_WINDOWS
#include <Windows.h>
#endif

int TSOMEntry(int argc, char* argv[], int(*mainFunc)(int argc, char* argv[]))
{
	fmt::print("TSOM {0}.{1}.{2} {3} ({4}) - {5}\n", tsom::GameMajorVersion, tsom::GameMinorVersion, tsom::GamePatchVersion, tsom::BuildBranch, tsom::BuildCommit, tsom::BuildDate);

#ifdef NAZARA_PLATFORM_WINDOWS
	if (IsDebuggerPresent())
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
