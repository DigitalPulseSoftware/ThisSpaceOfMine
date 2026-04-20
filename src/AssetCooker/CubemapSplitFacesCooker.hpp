// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_ASSETCOOKER_CUBEMAPSPLITFACESCOOKER_HPP
#define TSOM_ASSETCOOKER_CUBEMAPSPLITFACESCOOKER_HPP

#include <AssetCooker/Cooker.hpp>
#include <Nazara/Core/Enums.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <nlohmann/json_fwd.hpp>

namespace tsom
{
	class CubemapSplitFacesCooker : public Cooker
	{
		public:
			CubemapSplitFacesCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc);

			void Cook(Nz::TaskScheduler& taskScheduler) override;

			InputFileList GetInputFiles() const override;
			OutputFileList GetOutputFiles() const override;

		private:
			Nz::EnumArray<Nz::CubemapFace, std::filesystem::path> m_inputFiles;
			std::filesystem::path m_outputFile;
			bool m_sRGB;
	};
}

#endif // TSOM_ASSETCOOKER_CUBEMAPSPLITFACESCOOKER_HPP
