// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_ASSETCOOKER_BLOCKCOOKER_HPP
#define TSOM_ASSETCOOKER_BLOCKCOOKER_HPP

#include <AssetCooker/Cooker.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <nlohmann/json_fwd.hpp>

namespace tsom
{
	class BlockCooker : public Cooker
	{
		public:
			BlockCooker(const std::filesystem::path& inputFile, const std::filesystem::path& outputFile, const nlohmann::json& doc);

			void Cook(Nz::TaskScheduler& taskScheduler) override;

			InputFileList GetInputFiles() const override;
			OutputFileList GetOutputFiles() const override;

		private:
			enum class TextureType
			{
				AmbientOcclusion,
				BaseColor,
				Height,
				Metalness,
				Normal,
				Roughness,

				Max = Roughness
			};

			enum class CookedTextureType
			{
				BaseColor,
				Normal,
				MaterialData
			};

			struct InputData
			{
				Nz::EnumArray<TextureType, std::filesystem::path> files;
			};

			std::filesystem::path m_outputDir;
			std::vector<InputData> m_inputs;
			BlockLibrary m_blockLibrary;
			InputFileList m_inputFiles;
			OutputFileList m_outputFiles;
			Nz::UInt32 m_textureSize;
	};
}

#endif // TSOM_ASSETCOOKER_BLOCKCOOKER_HPP
