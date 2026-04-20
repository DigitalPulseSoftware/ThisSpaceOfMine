// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/CubemapCooker.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Core/ImageCompressor.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	CubemapCooker::CubemapCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_inputFile(inputDir / Nz::Utf8Path(doc.at("input").get<std::string>())),
	m_outputFile(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>()))
	{
	}

	void CubemapCooker::Cook(Nz::TaskScheduler& taskScheduler)
	{
		taskScheduler.AddTask([this]
		{
			std::filesystem::path outputDir = m_outputFile.parent_path();

			std::error_code ec;
			std::filesystem::create_directories(outputDir, ec);

			if (ec && ec != std::errc::is_a_directory)
			{
				spdlog::error("failed to create {}: {}", Nz::PathToString(outputDir), ec.message());
				return;
			}

			Nz::ImageParams imageParams;
			imageParams.loadFormat = Nz::PixelFormat::RGBA8_SRGB;

			std::shared_ptr<Nz::Image> inputImage = Nz::Image::LoadFromFile(m_inputFile, imageParams, Nz::CubemapParams{});
			if (!inputImage)
			{
				spdlog::error("failed to load image from {}", Nz::PathToString(m_inputFile));
				return;
			}

			if (!inputImage->GenerateMipmaps())
			{
				spdlog::error("failed to generate mipmaps for {}", Nz::PathToString(m_outputFile));
				return;
			}

			Nz::Image compressedImage = Nz::ImageCompressor::RGBA8ToBC1(*inputImage);
			if (!compressedImage.SaveToFile(m_outputFile))
			{
				spdlog::error("failed to save file to {}", Nz::PathToString(m_outputFile));
				return;
			}
		});
	}

	auto CubemapCooker::GetInputFiles() const -> InputFileList
	{
		return { m_inputFile };
	}

	auto CubemapCooker::GetOutputFiles() const -> OutputFileList
	{
		return { m_outputFile };
	}
}
