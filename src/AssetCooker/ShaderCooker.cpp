// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/ShaderCooker.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <process.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	ShaderCooker::ShaderCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_inputFile(inputDir / Nz::Utf8Path(doc.at("input").get<std::string>())),
	m_outputFile(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>()))
	{
	}

	void ShaderCooker::Cook(Nz::TaskScheduler& taskScheduler)
	{
		if (m_inputFile.stem() != m_outputFile.stem())
		{
			spdlog::error("currently the input file and the output file must have the same name");
			return;
		}

		if (m_outputFile.extension() != Nz::Utf8Path(".nzslb"))
		{
			spdlog::error("only .nzslb output extension is supported");
			return;
		}

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

			std::string output;
			auto ReadStdout = [&](const char* str, std::size_t size)
			{
				output.append(str, size);
			};

			std::string errOutput;
			auto ReadStderr = [&](const char* str, std::size_t size)
			{
				errOutput.append(str, size);
			};

			TinyProcessLib::Process process({ "nzslc", "--compile", "--partial", "--output", Nz::PathToString(outputDir), Nz::PathToString(m_inputFile) }, {}, ReadStdout, ReadStderr);
			int exitCode = process.get_exit_status();
			if (exitCode != 0)
			{
				spdlog::error("failed to compile {}: {}", Nz::PathToString(m_inputFile), errOutput);
				return;
			}
		});
	}

	auto ShaderCooker::GetInputFiles() const -> InputFileList
	{
		return { m_inputFile };
	}

	auto ShaderCooker::GetOutputFiles() const -> OutputFileList
	{
		return { m_outputFile };
	}
}
