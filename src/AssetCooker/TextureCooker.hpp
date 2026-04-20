// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_ASSETCOOKER_TEXTURECOOKER_HPP
#define TSOM_ASSETCOOKER_TEXTURECOOKER_HPP

#include <AssetCooker/Cooker.hpp>
#include <nlohmann/json_fwd.hpp>
#include <array>

namespace tsom
{
	class TextureCooker : public Cooker
	{
		public:
			enum class ChannelSource
			{
				Red,
				Green,
				Blue,
				Alpha
			};

			enum class TextureType
			{
				Color,
				Normal,
				Greyscale,
				BiGreyscale
			};

			TextureCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc);

			void Cook(Nz::TaskScheduler& taskScheduler) override;

			InputFileList GetInputFiles() const override;
			OutputFileList GetOutputFiles() const override;

		private:
			std::array<ChannelSource, 4> m_channelSources;
			std::filesystem::path m_inputFile;
			std::filesystem::path m_outputFile;
			bool m_compress;
			bool m_generateMipmaps;
			TextureType m_textureType;
	};
}

#endif // TSOM_ASSETCOOKER_TEXTURECOOKER_HPP
