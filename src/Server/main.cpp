// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/HealthCheckerAppComponent.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <Server/ServerConfigAppComponent.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <ServerLib/ServerPlanetEnvironment.hpp>
#include <ServerLib/Components/EnvironmentProxyComponent.hpp>
#include <ServerLib/Components/NetworkedComponent.hpp>
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

#include <ServerLib/Components/EnvironmentEnterTriggerComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::Physics3D, Nz::Network> app(argc, argv);

	app.AddComponent<Nz::SignalHandlerAppComponent>();
	app.AddComponent<Nz::TaskSchedulerAppComponent>();
	app.AddComponent<Nz::WebServiceAppComponent>();
	app.AddComponent<tsom::PlayerTokenAppComponent>();
	auto& configAppComponent = app.AddComponent<tsom::ServerConfigAppComponent>();
	auto& serverInstanceAppComponent = app.AddComponent<tsom::ServerInstanceAppComponent>();

	auto& filesystem = app.AddComponent<Nz::FilesystemAppComponent>();
	for (const char* directory : { "database", "scripts" })
	{
		std::filesystem::path dirPath = Nz::Utf8Path(directory);
		if (!std::filesystem::is_directory(dirPath))
		{
			fmt::print(fg(fmt::color::red), "{0} directory is missing!\n", directory);
			return EXIT_FAILURE;
		}

		filesystem.Mount(directory, dirPath);
	}

	auto& config = configAppComponent.GetConfig();

	Nz::UInt16 serverPort = config.GetIntegerValue<Nz::UInt16>("Server.Port");
	std::filesystem::path saveDirectory = Nz::Utf8Path(config.GetStringValue("Save.Directory"));

	tsom::ServerInstance::Config instanceConfig;
	instanceConfig.pauseWhenEmpty = config.GetBoolValue("Server.SleepWhenEmpty");
	instanceConfig.saveInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>("Save.Interval"));
	instanceConfig.connectionTokenEncryptionKey = config.GetConnectionTokenEncryptionKey();
	instanceConfig.enableDebugDrawer = config.GetBoolValue("Debug.EnableDrawer");
	instanceConfig.databaseFile = config.GetStringValue("Database.Filename");

	auto& instance = serverInstanceAppComponent.AddInstance(instanceConfig);
	auto& sessionManager = instance.AddSessionManager(serverPort);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	instance.LoadFromDatabase();
	/*tsom::ServerPlanetEnvironment planet(instance, "alice", saveDirectory / Nz::Utf8Path("alice"), 42, Nz::Vector3ui(5), 1.f);
	instance.SetDefaultSpawnpoint(&planet, Nz::Vector3f::Up() * 100.f + Nz::Vector3f::Backward() * 5.f, Nz::Quaternionf::Identity());

	tsom::ServerPlanetEnvironment planet2(instance, "bob", saveDirectory / Nz::Utf8Path("bob"), 41, Nz::Vector3ui(5), 1.f, 40.f);

	entt::handle switchToPlanet2Entity = planet.CreateEntity();
	auto& enterPlanet2Trigger = switchToPlanet2Entity.emplace<tsom::EnvironmentEnterTriggerComponent>();
	enterPlanet2Trigger.aabb = Nz::Boxf(-300.f, -300.f, -300.f, 600.f, 600.f, 600.f);
	enterPlanet2Trigger.targetEnvironment = &planet2;
	enterPlanet2Trigger.updateRoot = true;

	switchToPlanet2Entity.emplace<Nz::NodeComponent>(Nz::Vector3f(-10000.f, 0.f, 0.f));
	switchToPlanet2Entity.emplace<tsom::EnvironmentProxyComponent>().targetEnvironment = &planet2;
	switchToPlanet2Entity.emplace<tsom::NetworkedComponent>();

	entt::handle switchToPlanet1Entity = planet2.CreateEntity();
	auto& enterPlanet1Trigger = switchToPlanet1Entity.emplace<tsom::EnvironmentEnterTriggerComponent>();
	enterPlanet1Trigger.aabb = Nz::Boxf(-300.f, -300.f, -300.f, 600.f, 600.f, 600.f);
	switchToPlanet1Entity.emplace<Nz::NodeComponent>(Nz::Vector3f(10000.f, 0.f, 0.f));
	switchToPlanet1Entity.emplace<tsom::EnvironmentProxyComponent>().targetEnvironment = &planet;
	switchToPlanet1Entity.emplace<tsom::NetworkedComponent>();
	enterPlanet1Trigger.targetEnvironment = &planet;
	enterPlanet1Trigger.updateRoot = true;*/

	fmt::print(fg(fmt::color::lime_green), "server ready.\n");

	if (Nz::UInt32 maxStuckTime = config.GetIntegerValue<Nz::UInt32>("Server.MaxStuckSeconds"))
		app.AddComponent<tsom::HealthCheckerAppComponent>(maxStuckTime);

	return app.Run();
}

TSOMMain(ServerMain)
