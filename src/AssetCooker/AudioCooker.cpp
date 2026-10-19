// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/AudioCooker.hpp>
#include <Nazara/Audio/SoundBuffer.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	AudioCooker::AudioCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_inputFile(inputDir / Nz::Utf8Path(doc.at("input").get<std::string>())),
	m_outputFile(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>()))
	{
	}

	void AudioCooker::Cook(Nz::TaskScheduler& taskScheduler)
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

			std::shared_ptr<Nz::SoundBuffer> soundBuffer = Nz::SoundBuffer::LoadFromFile(m_inputFile);
			if (!soundBuffer)
			{
				spdlog::error("failed to decode {}", Nz::PathToString(m_inputFile));
				return;
			}

			if (!soundBuffer->SaveToFile(m_outputFile))
			{
				spdlog::error("failed to re-encode to {}", Nz::PathToString(m_outputFile));
				return;
			}
		});
	}

	auto AudioCooker::GetInputFiles() const -> InputFileList
	{
		return { m_inputFile };
	}

	auto AudioCooker::GetOutputFiles() const -> OutputFileList
	{
		return { m_outputFile };
	}
}
