// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/HealthCheckerAppComponent.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <CommonLib/Utility/JsonSerialization.hpp>
#include <Server/ServerConfigAppComponent.hpp>
#include <Server/ServerConfigs.hpp>
#include <Server/ServerUpdateAppComponent.hpp>
#include <ServerLib/PlayerTokenAppComponent.hpp>
#include <ServerLib/ServerInstanceAppComponent.hpp>
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
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
	for (const char* directory : { "CookedAssets", "database", "scripts" })
	{
		std::filesystem::path dirPath = Nz::Utf8Path(directory);
		if (!std::filesystem::is_directory(dirPath))
		{
			spdlog::error("{0} directory is missing!", directory);
			return EXIT_FAILURE;
		}

		filesystem.Mount(directory, dirPath);
	}

	auto& config = configAppComponent.GetConfig();

	if (config.GetBoolValue(tsom::Config::Server_AutoUpdater_Enabled))
	{
		app.AddComponent<tsom::UpdaterAppComponent>(config);

		tsom::ServerUpdateAppComponent::UpdateBehavior updateBehavior;
		const std::string& updateBehaviorStr = config.GetStringValue(tsom::Config::Server_AutoUpdater_Behavior);
		if (updateBehaviorStr == "downloadandexit")
			updateBehavior = tsom::ServerUpdateAppComponent::UpdateBehavior::DownloadAndExit;
		else
		{
			NazaraAssertMsg(updateBehaviorStr == "downloadandupdate", "unexpected update behavior %s", updateBehaviorStr.c_str());
			updateBehavior = tsom::ServerUpdateAppComponent::UpdateBehavior::DownloadAndUpdate;
		}

		Nz::Time checkInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>(tsom::Config::Server_AutoUpdater_CheckInterval));
		Nz::Time quitDelay = Nz::Time::Seconds(config.GetIntegerValue<long long>(tsom::Config::Server_AutoUpdater_QuitDelay));

		app.AddComponent<tsom::ServerUpdateAppComponent>(updateBehavior, checkInterval, quitDelay);
	}

	Nz::UInt16 serverPort = config.GetIntegerValue<Nz::UInt16>(tsom::Config::Server_Port);

	tsom::ServerInstance::Config instanceConfig;
	instanceConfig.pauseWhenEmpty = config.GetBoolValue(tsom::Config::Server_SleepWhenEmpty);
	instanceConfig.saveInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>(tsom::Config::Save_Interval));
	instanceConfig.connectionTokenEncryptionKey = config.GetConnectionTokenEncryptionKey();
	instanceConfig.enableChunkCache = config.GetBoolValue(tsom::Config::Chunk_EnableCache);
	instanceConfig.enableDebugDrawer = config.GetBoolValue(tsom::Config::Debug_EnableDrawer);
	instanceConfig.databaseFile = config.GetStringValue(tsom::Config::Database_Filename);

	auto& instance = serverInstanceAppComponent.AddInstance(instanceConfig);
	auto& sessionManager = instance.AddSessionManager(serverPort);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	if (!std::filesystem::is_regular_file(instanceConfig.databaseFile))
	{
		// No save exists, create a default planet
		auto& serverDatabase = instance.GetThreadServerDatabase();
		serverDatabase.StorePlanet({
			.id = 1,
			.generatorName = "bob",
			.type = "round_cube_planet",
			.chunkCount = Nz::Vector3ui(15),
			.properties = nlohmann::json{
				{ "BlockSize", 0.5f },
				{ "CornerRadius", 16.f },
				{ "Gravity", 9.81f },
				{ "Seed", 42 },
				{ "Atmosphere", nlohmann::json {
					{ "Shape", "RoundCube" },
					{ "ShapeSettings", Nz::Vector4f(60.f, 60.f, 60.f, 16.f) },
					{ "MaxHeight", 180.f },
					{ "ScatteringStrength", 0.5f },
					{ "WaveLengths", Nz::Vector3f(700.0f, 530.0f, 440.0f) },
					{ "MieScattering", 0.9f },
					{ "MieHeight", 5.0f },
					{ "DensityFalloff", 20.0f }
				} }
			}
		});
	}

	instance.LoadFromDatabase();

	spdlog::info("server ready.");

	if (Nz::UInt32 maxStuckTime = config.GetIntegerValue<Nz::UInt32>(tsom::Config::Server_MaxStuckSeconds))
		app.AddComponent<tsom::HealthCheckerAppComponent>(maxStuckTime);

	return app.Run();
}

TSOMMain(ServerMain)
