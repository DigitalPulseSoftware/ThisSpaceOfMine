// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_ASSETCOOKER_SHADERCOOKER_HPP
#define TSOM_ASSETCOOKER_SHADERCOOKER_HPP

#include <AssetCooker/Cooker.hpp>
#include <nlohmann/json_fwd.hpp>

namespace tsom
{
	class ShaderCooker : public Cooker
	{
		public:
			ShaderCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc);

			void Cook(Nz::TaskScheduler& taskScheduler) override;

			InputFileList GetInputFiles() const override;
			OutputFileList GetOutputFiles() const override;

		private:
			std::filesystem::path m_inputFile;
			std::filesystem::path m_outputFile;
	};
}

#endif // TSOM_ASSETCOOKER_SHADERCOOKER_HPP
