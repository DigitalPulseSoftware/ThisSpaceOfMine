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
			enum class SourceChannel
			{
				Red,
				InvRed,
				Green,
				InvGreen,
				Blue,
				InvBlue,
				Alpha,
				InvAlpha,

				Max = InvAlpha
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
			std::array<SourceChannel, 4> m_sourceChannel;
			std::array<Nz::UInt32, 4> m_sourceTexture;
			std::array<std::filesystem::path, 4> m_inputFiles;
			std::filesystem::path m_outputFile;
			bool m_compress;
			bool m_generateMipmaps;
			TextureType m_textureType;
	};
}

#endif // TSOM_ASSETCOOKER_TEXTURECOOKER_HPP
