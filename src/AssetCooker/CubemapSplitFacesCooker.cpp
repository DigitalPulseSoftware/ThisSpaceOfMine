// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/CubemapSplitFacesCooker.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Core/ImageCompressor.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	CubemapSplitFacesCooker::CubemapSplitFacesCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_outputFile(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>())),
	m_sRGB(doc.value("sRGB", true))
	{
		nlohmann::json faces = doc["faces"];
		m_inputFiles[Nz::CubemapFace::PositiveX] = inputDir / Nz::Utf8Path(faces.at("+x").get<std::string>());
		m_inputFiles[Nz::CubemapFace::NegativeX] = inputDir / Nz::Utf8Path(faces.at("-x").get<std::string>());
		m_inputFiles[Nz::CubemapFace::PositiveY] = inputDir / Nz::Utf8Path(faces.at("+y").get<std::string>());
		m_inputFiles[Nz::CubemapFace::NegativeY] = inputDir / Nz::Utf8Path(faces.at("-y").get<std::string>());
		m_inputFiles[Nz::CubemapFace::PositiveZ] = inputDir / Nz::Utf8Path(faces.at("+z").get<std::string>());
		m_inputFiles[Nz::CubemapFace::NegativeZ] = inputDir / Nz::Utf8Path(faces.at("-z").get<std::string>());
	}

	void CubemapSplitFacesCooker::Cook(Nz::TaskScheduler& taskScheduler)
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

			Nz::EnumArray<Nz::CubemapFace, std::shared_ptr<Nz::Image>> faceImages;

			Nz::ImageParams imageParams;
			imageParams.loadFormat = (m_sRGB) ? Nz::PixelFormat::RGBA8_SRGB : Nz::PixelFormat::RGBA8;

			for (auto&& [cubemapFace, path] : m_inputFiles.iter_kv())
			{
				faceImages[cubemapFace] = Nz::Image::LoadFromFile(path, imageParams);
				if (!faceImages[cubemapFace])
				{
					spdlog::error("failed to load image from {}", Nz::PathToString(path));
					return;
				}
			}

			const Nz::Image& referenceImage = *faceImages.front();
			Nz::Image image(Nz::ImageType::Cubemap, imageParams.loadFormat, referenceImage.GetWidth(), referenceImage.GetHeight());
			for (auto&& [cubemapFace, faceImage] : faceImages.iter_kv())
			{
				if (!image.LoadFaceFromImage(cubemapFace, *faceImage))
				{
					spdlog::error("failed to load face image from {}", Nz::PathToString(m_inputFiles[cubemapFace]));
					return;
				}
			}

			if (!image.GenerateMipmaps())
			{
				spdlog::error("failed to generate mipmaps for {}", Nz::PathToString(m_outputFile));
				return;
			}

			image = Nz::ImageCompressor::RGBA8ToBC1(image);

			if (!image.SaveToFile(m_outputFile))
			{
				spdlog::error("failed to save file to {}", Nz::PathToString(m_outputFile));
				return;
			}
		});
	}

	auto CubemapSplitFacesCooker::GetInputFiles() const -> InputFileList
	{
		return { m_inputFiles.begin(), m_inputFiles.end() };
	}

	auto CubemapSplitFacesCooker::GetOutputFiles() const -> OutputFileList
	{
		return { m_outputFile };
	}
}
