// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/HealthCheckerAppComponent.hpp>
#include <CommonLib/InternalConstants.hpp>
#include <CommonLib/UpdaterAppComponent.hpp>
#include <Server/ServerConfigAppComponent.hpp>
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
#include <spdlog/spdlog.h>

void MigrateFileToSqlite(tsom::ServerInstance& instance, const std::filesystem::path& saveDirectory);
void MigratePlanetChunksToSqlite(const std::filesystem::path& saveDirectory, tsom::ServerDatabase& serverDatabase, Nz::UInt32 planetId, std::string_view planetName);

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
			spdlog::error("{0} directory is missing!", directory);
			return EXIT_FAILURE;
		}

		filesystem.Mount(directory, dirPath);
	}

	auto& config = configAppComponent.GetConfig();

	if (config.GetBoolValue("Server.AutoUpdater.Enabled"))
	{
		app.AddComponent<tsom::UpdaterAppComponent>(config);

		tsom::ServerUpdateAppComponent::UpdateBehavior updateBehavior;
		const std::string& updateBehaviorStr = config.GetStringValue("Server.AutoUpdater.Behavior");
		if (updateBehaviorStr == "downloadandexit")
			updateBehavior = tsom::ServerUpdateAppComponent::UpdateBehavior::DownloadAndExit;
		else
		{
			NazaraAssertMsg(updateBehaviorStr == "downloadandupdate", "unexpected update behavior %s", updateBehaviorStr.c_str());
			updateBehavior = tsom::ServerUpdateAppComponent::UpdateBehavior::DownloadAndUpdate;
		}

		Nz::Time checkInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>("Server.AutoUpdater.CheckInterval"));
		Nz::Time quitDelay = Nz::Time::Seconds(config.GetIntegerValue<long long>("Server.AutoUpdater.QuitDelay"));

		app.AddComponent<tsom::ServerUpdateAppComponent>(updateBehavior, checkInterval, quitDelay);
	}

	Nz::UInt16 serverPort = config.GetIntegerValue<Nz::UInt16>("Server.Port");

	tsom::ServerInstance::Config instanceConfig;
	instanceConfig.pauseWhenEmpty = config.GetBoolValue("Server.SleepWhenEmpty");
	instanceConfig.saveInterval = Nz::Time::Seconds(config.GetIntegerValue<long long>("Save.Interval"));
	instanceConfig.connectionTokenEncryptionKey = config.GetConnectionTokenEncryptionKey();
	instanceConfig.enableDebugDrawer = config.GetBoolValue("Debug.EnableDrawer");
	instanceConfig.databaseFile = config.GetStringValue("Database.Filename");

	bool isMigratingToDatabase = !std::filesystem::is_regular_file(instanceConfig.databaseFile);

	auto& instance = serverInstanceAppComponent.AddInstance(instanceConfig);
	auto& sessionManager = instance.AddSessionManager(serverPort);
	sessionManager.SetDefaultHandler<tsom::InitialSessionHandler>(std::ref(instance));

	std::filesystem::path saveDirectory = Nz::Utf8Path(config.GetStringValue("Save.Directory"));

	if (isMigratingToDatabase)
	{
		if (std::filesystem::is_directory(saveDirectory))
		{
			spdlog::warn("first time launching after sqlite switch: migrating saves.");

			MigrateFileToSqlite(instance, saveDirectory);
			std::filesystem::path migratedSaveDir = saveDirectory;
			migratedSaveDir += Nz::Utf8Path("_migrated");

			std::filesystem::rename(saveDirectory, migratedSaveDir);

			spdlog::info("save migrated.");
		}
		else
		{
			// No save exists, create a default planet
			auto& serverDatabase = instance.GetServerDatabase();
			serverDatabase.StorePlanet({
				.id = 1,
				.generatorName = "bob",
				.seed = 42,
				.chunkCount = Nz::Vector3ui(5),
				.cornerRadius = 16.f,
				.gravity = 9.81f
			});
		}
	}

	instance.LoadFromDatabase();

	spdlog::info("server ready.");

	if (Nz::UInt32 maxStuckTime = config.GetIntegerValue<Nz::UInt32>("Server.MaxStuckSeconds"))
		app.AddComponent<tsom::HealthCheckerAppComponent>(maxStuckTime);

	return app.Run();
}

TSOMMain(ServerMain)

void MigrateFileToSqlite(tsom::ServerInstance& instance, const std::filesystem::path& saveDirectory)
{
	auto& serverDatabase = instance.GetServerDatabase();

	serverDatabase.StorePlanet({
		.id = 1,
		.generatorName = "alice",
		.seed = 42,
		.chunkCount = Nz::Vector3ui(5),
		.cornerRadius = 16.f,
		.gravity = 9.81f
	});

	serverDatabase.StorePlanet({
		.id = 2,
		.generatorName = "bob",
		.seed = 41,
		.chunkCount = Nz::Vector3ui(5),
		.cornerRadius = 40.f,
		.gravity = 9.81f
	});

	serverDatabase.StorePlanetLink({
		.sourcePlanet = 1,
		.destinationPlanet = 2,
		.position = Nz::Vector3f(-10000.f, 0.f, 0.f)
	});

	serverDatabase.StorePlanetLink({
		.sourcePlanet = 2,
		.destinationPlanet = 1,
		.position = Nz::Vector3f(10000.f, 0.f, 0.f)
	});

	MigratePlanetChunksToSqlite(saveDirectory, serverDatabase, 1, "alice");
	MigratePlanetChunksToSqlite(saveDirectory, serverDatabase, 2, "bob");
}

void MigratePlanetChunksToSqlite(const std::filesystem::path& saveDirectory, tsom::ServerDatabase& serverDatabase, Nz::UInt32 planetId, std::string_view planetName)
{
	for (std::filesystem::path chunkFile : std::filesystem::directory_iterator(saveDirectory / Nz::Utf8Path(planetName)))
	{
		if (chunkFile.extension() != Nz::Utf8Path(".chunk"))
			continue;

		std::string fileName = Nz::PathToString(chunkFile.filename());
		unsigned int x, y, z;
		if (std::sscanf(fileName.c_str(), "%u_%u_%u.chunk", &x, &y, &z) != 3)
		{
			spdlog::error("failed to load planet {0} chunk: failed to parse chunk name {1}", planetName, fileName);
			continue;
		}

		std::optional chunkBuffer = Nz::File::ReadWhole(chunkFile);
		if (!chunkBuffer)
		{
			spdlog::error("failed to load planet {0} chunk: failed to load chunk file {1}", planetName, fileName);
			continue;
		}

		Nz::UInt32 decompressedSize = Nz::SafeCaster(chunkBuffer->size());

		tsom::BinaryCompressor& binaryCompressor = tsom::BinaryCompressor::GetThreadCompressor();
		std::optional compressedDataOpt = binaryCompressor.Compress(chunkBuffer->data(), chunkBuffer->size());
		if NAZARA_UNLIKELY(!compressedDataOpt)
			throw std::runtime_error("chunk compression failed");

		std::span<Nz::UInt8>& compressedData = *compressedDataOpt;

		// Reuse byteArray
		std::vector<Nz::UInt8> chunkData(sizeof(Nz::UInt32) + compressedData.size());

		decompressedSize = Nz::HostToLittleEndian(decompressedSize);
		std::memcpy(&chunkData[0], &decompressedSize, sizeof(decompressedSize));
		std::memcpy(&chunkData[sizeof(decompressedSize)], &compressedData[0], compressedData.size());

		serverDatabase.StorePlanetChunk(tsom::Database::PlanetChunk{
			.planetId = planetId,
			.position = tsom::ChunkIndices(x, y, z),
			.version = 1,
			.chunkData = chunkData
		});
	}
}
