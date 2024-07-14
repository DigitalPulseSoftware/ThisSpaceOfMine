// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/InternalConstants.hpp>
#include <Server/ServerConfigAppComponent.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
#include <ServerLib/Session/InitialSessionHandler.hpp>
#include <Nazara/Core/Application.hpp>
#include <Nazara/Core/Core.hpp>
#include <Nazara/Core/SignalHandlerAppComponent.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Physics3D/Physics3D.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <Main/Main.hpp>
#include <fmt/color.h>

int ServerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::Physics3D, Nz::Network> app(argc, argv);

	app.AddComponent<Nz::SignalHandlerAppComponent>();
	app.AddComponent<Nz::TaskSchedulerAppComponent>();
	auto& configAppComponent = app.AddComponent<tsom::ServerConfigAppComponent>();
	auto& worldAppComponent = app.AddComponent<tsom::ServerInstanceAppComponent>();

	auto& config = configAppComponent.GetConfig();

	Nz::UInt16 serverPort = config.GetIntegerValue<Nz::UInt16>("Server.Port");

	tsom::ServerInstance::Config instanceConfig;
	instanceConfig.pauseWhenEmpty = config.GetBoolValue("Server.SleepWhenEmpty");
	instanceConfig.saveDirectory = Nz::Utf8Path(config.GetStringValue("Save.Directory"));
	instanceConfig.saveInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>("Save.Interval"));
	instanceConfig.connectionTokenEncryptionKey = config.GetConnectionTokenEncryptionKey();

	auto& instance = worldAppComponent.AddInstance(std::move(instanceConfig));
	auto& sessionManager = instance.AddSessionManager(serverPort);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	fmt::print(fg(fmt::color::lime_green), "server ready.\n");

	return app.Run();
}

TSOMMain(ServerMain)
