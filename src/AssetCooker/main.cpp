// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/BlockCooker.hpp>
#include <AssetCooker/CopyCooker.hpp>
#include <AssetCooker/CubemapCooker.hpp>
#include <AssetCooker/CubemapSplitFacesCooker.hpp>
#include <AssetCooker/ShaderCooker.hpp>
#include <AssetCooker/TextureCooker.hpp>
#include <Nazara/Core/Application.hpp>
#include <Nazara/Core/Core.hpp>
#include <Nazara/Core/ImageCompressor.hpp>
#include <Nazara/Core/TaskSchedulerAppComponent.hpp>
#include <Nazara/Network/Network.hpp>
#include <Nazara/Physics3D/Physics3D.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <Main/Main.hpp>
#include <frozen/string.h>
#include <frozen/unordered_map.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

using CookMethodBuilder = std::unique_ptr<tsom::Cooker>(*)(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc);

struct CookMethodData
{
	CookMethodBuilder builder;

	template<typename T>
	static constexpr CookMethodData Build()
	{
		return CookMethodData {
			.builder = [](const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) -> std::unique_ptr<tsom::Cooker> { return std::make_unique<T>(inputDir, outputDir, doc); }
		};
	}
};

constexpr auto s_cookMethods = frozen::make_unordered_map<frozen::string, CookMethodData>({
	{ "Blocks", CookMethodData::Build<tsom::BlockCooker>() },
	{ "Copy", CookMethodData::Build<tsom::CopyCooker>() },
	{ "Cubemap", CookMethodData::Build<tsom::CubemapCooker>() },
	{ "CubemapSplitFaces", CookMethodData::Build<tsom::CubemapSplitFacesCooker>() },
	{ "Shader", CookMethodData::Build<tsom::ShaderCooker>() },
	{ "Texture", CookMethodData::Build<tsom::TextureCooker>() }
});

int CookerMain(int argc, char* argv[])
{
	Nz::Application<Nz::Core, Nz::Physics3D, Nz::Network> app(argc, argv);
	auto& taskScheduler = app.AddComponent<Nz::TaskSchedulerAppComponent>(4);

	std::filesystem::path sourcePath = Nz::Utf8Path("assets");
	std::filesystem::path destinationPath = Nz::Utf8Path("cache/CookedAssets");

	std::ifstream assetFile(sourcePath / Nz::Utf8Path("assets.json"));
	nlohmann::json assetListDoc = nlohmann::json::parse(assetFile);

	std::vector<std::unique_ptr<tsom::Cooker>> cookers;
	for (const nlohmann::json& cookDoc : assetListDoc["assets"])
	{
		const std::string& method = cookDoc["method"];

		auto cookMethodIt = s_cookMethods.find(frozen::string(method));
		if (cookMethodIt == s_cookMethods.end())
		{
			spdlog::error("invalid method \"{}\"", method);
			continue;
		}

		const CookMethodData& cookMethodData = cookMethodIt->second;

		std::unique_ptr<tsom::Cooker> cooker = cookMethodData.builder(sourcePath, destinationPath, cookDoc);

		std::filesystem::file_time_type outputTime;

		bool canSkip = true;
		tsom::Cooker::OutputFileList outputFiles = cooker->GetOutputFiles();
		for (const std::filesystem::path& outputFile : outputFiles)
		{
			std::error_code ec;
			std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(outputFile, ec);

			if (ec)
			{
				if (ec != std::errc::no_such_file_or_directory)
					spdlog::warn("failed to get mtime of output file {}: {}", Nz::PathToString(outputFile), ec.message());

				canSkip = false;
				break;
			}

			outputTime = std::max(outputTime, lastWriteTime);
		}

		if (canSkip)
		{
			for (const std::filesystem::path& inputFile : cooker->GetInputFiles())
			{
				std::error_code ec;
				std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(inputFile, ec);

				if (ec)
				{
					spdlog::error("failed to get mtime of input file {}: {}", Nz::PathToString(inputFile), ec.message());
					canSkip = false;
					break;
				}

				if (lastWriteTime > outputTime)
				{
					spdlog::debug("{} mtime is greater than output mtime");
					canSkip = false;
					break;
				}
			}
		}

		if (!canSkip)
		{
			spdlog::info("processing {}", Nz::PathToString(cooker->GetInputFiles().front()));
			cooker->Cook(taskScheduler);
			cookers.emplace_back(std::move(cooker));
		}
		else
			spdlog::info("skipped {} cook (up to date)", Nz::PathToString(outputFiles.front()));
	}

	taskScheduler.WaitForTasks();
	return 0;
}

TSOMMain(CookerMain)
