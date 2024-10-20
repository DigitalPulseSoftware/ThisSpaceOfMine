// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/HealthCheckerAppComponent.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Server/ServerConfigAppComponent.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <Nazara/Core/Application.hpp>
#include <Nazara/Core/Core.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/SignalHandlerAppComponent.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Network/WebServiceAppComponent.hpp>
#include <Nazara/Physics3D/Physics3D.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <Main/Main.hpp>
#include <fmt/color.h>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::Physics3D, Nz::Network> app(argc, argv);

	app.AddComponent<Nz::SignalHandlerAppComponent>();
	app.AddComponent<Nz::TaskSchedulerAppComponent>();
	app.AddComponent<Nz::WebServiceAppComponent>();
	app.AddComponent<tsom::PlayerTokenAppComponent>();
	auto& configAppComponent = app.AddComponent<tsom::ServerConfigAppComponent>();
	auto& worldAppComponent = app.AddComponent<tsom::ServerInstanceAppComponent>();

	std::filesystem::path scriptPath = Nz::Utf8Path("scripts");
	if (!std::filesystem::is_directory(scriptPath))
	{
		fmt::print(fg(fmt::color::red), "scripts are missing!\n");
		return EXIT_FAILURE;
	}

	auto& filesystem = app.AddComponent<Nz::FilesystemAppComponent>();
	filesystem.Mount("scripts", scriptPath);

	auto& config = configAppComponent.GetConfig();

	if (Nz::UInt32 maxStuckTime = config.GetIntegerValue<Nz::UInt32>("Server.MaxStuckSeconds"))
		app.AddComponent<tsom::HealthCheckerAppComponent>(maxStuckTime);

	Nz::UInt16 serverPort = config.GetIntegerValue<Nz::UInt16>("Server.Port");
	std::filesystem::path saveDirectory = Nz::Utf8Path(config.GetStringValue("Save.Directory"));

	tsom::ServerInstance::Config instanceConfig;
	instanceConfig.pauseWhenEmpty = config.GetBoolValue("Server.SleepWhenEmpty");
	instanceConfig.saveInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>("Save.Interval"));
	instanceConfig.connectionTokenEncryptionKey = config.GetConnectionTokenEncryptionKey();

	auto& instance = worldAppComponent.AddInstance(instanceConfig);
	auto& sessionManager = instance.AddSessionManager(serverPort);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	tsom::ServerPlanetEnvironment planet(instance, saveDirectory, 42, Nz::Vector3ui(5));
	instance.SetDefaultSpawnpoint(&planet, Nz::Vector3f::Up() * 100.f + Nz::Vector3f::Backward() * 5.f, Nz::Quaternionf::Identity());

	fmt::print(fg(fmt::color::lime_green), "server ready.\n");

	return app.Run();
}

TSOMMain(ServerMain)
