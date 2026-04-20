// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/TextureCooker.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Core/ImageCompressor.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace nlohmann
{
	NLOHMANN_JSON_SERIALIZE_ENUM(tsom::TextureCooker::ChannelSource, {
		{tsom::TextureCooker::ChannelSource::Red, "Red"},
		{tsom::TextureCooker::ChannelSource::Green, "Green"},
		{tsom::TextureCooker::ChannelSource::Blue, "Blue"},
		{tsom::TextureCooker::ChannelSource::Alpha, "Alpha"}
	});

	NLOHMANN_JSON_SERIALIZE_ENUM(tsom::TextureCooker::TextureType, {
		{tsom::TextureCooker::TextureType::Color, "Color"},
		{tsom::TextureCooker::TextureType::Normal, "Normal"},
		{tsom::TextureCooker::TextureType::Greyscale, "Greyscale"},
		{tsom::TextureCooker::TextureType::BiGreyscale, "BiGreyscale"}
	});
}

namespace tsom
{
	TextureCooker::TextureCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_inputFile(inputDir / Nz::Utf8Path(doc.at("input").get<std::string>())),
	m_outputFile(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>())),
	m_compress(doc.value("compress", true)),
	m_generateMipmaps(doc.value("generateMipmaps", true)),
	m_textureType(doc.value("type", TextureType::Color))
	{
		m_channelSources[0] = doc.value("channel0", ChannelSource::Red);
		m_channelSources[1] = doc.value("channel1", ChannelSource::Green);
		m_channelSources[2] = doc.value("channel2", ChannelSource::Blue);
		m_channelSources[3] = doc.value("channel3", ChannelSource::Alpha);
	}

	void TextureCooker::Cook(Nz::TaskScheduler& taskScheduler)
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

			bool noSwizzling = m_channelSources[0] == ChannelSource::Red && m_channelSources[1] == ChannelSource::Green && m_channelSources[2] == ChannelSource::Blue && m_channelSources[3] == ChannelSource::Alpha;

			Nz::ImageParams imageParams;
			switch (m_textureType)
			{
				case TextureType::Color:
					imageParams.loadFormat = Nz::PixelFormat::RGBA8_SRGB;
					break;

				case TextureType::Greyscale:
					imageParams.loadFormat = (noSwizzling) ? Nz::PixelFormat::R8 : Nz::PixelFormat::RGBA8;
					break;

				case TextureType::BiGreyscale:
					imageParams.loadFormat = (noSwizzling) ? Nz::PixelFormat::RG8 : Nz::PixelFormat::RGBA8;
					break;

				case TextureType::Normal:
					imageParams.loadFormat = (noSwizzling) ? Nz::PixelFormat::RGB8 : Nz::PixelFormat::RGBA8;
					break;
			}

			std::shared_ptr<Nz::Image> inputImage = Nz::Image::LoadFromFile(m_inputFile, imageParams);
			if (!inputImage)
			{
				spdlog::error("failed to load image from {}", Nz::PathToString(m_inputFile));
				return;
			}

			Nz::Image cookedImage;
			switch (m_textureType)
			{
				case TextureType::Color:
				{
					if (noSwizzling)
					{
						// Image is already good
						cookedImage = std::move(*inputImage);
						inputImage.reset();
						break;
					}

					// Apply swizzling
					Nz::UInt32 width = inputImage->GetWidth();
					Nz::UInt32 height = inputImage->GetHeight();
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::RGBA8, width, height);

					const Nz::UInt8* sourcePixels = inputImage->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedImage.GetPixels();
					Nz::UInt8 bpp = Nz::PixelFormatInfo::GetBytesPerPixel(inputImage->GetFormat());

					Nz::UInt32 channelR = Nz::UnderlyingCast(m_channelSources[0]);
					Nz::UInt32 channelG = Nz::UnderlyingCast(m_channelSources[1]);
					Nz::UInt32 channelB = Nz::UnderlyingCast(m_channelSources[2]);
					Nz::UInt32 channelA = Nz::UnderlyingCast(m_channelSources[3]);

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							cookedPixels[0] = sourcePixels[channelR];
							cookedPixels[1] = sourcePixels[channelG];
							cookedPixels[2] = sourcePixels[channelB];
							cookedPixels[3] = sourcePixels[channelA];

							sourcePixels += bpp;
							cookedPixels += 4;
						}
					}

					break;
				}

				case TextureType::Greyscale:
				{
					if (noSwizzling)
					{
						// Image is already good
						cookedImage = std::move(*inputImage);
						inputImage.reset();
						break;
					}

					// Apply swizzling
					Nz::UInt32 width = inputImage->GetWidth();
					Nz::UInt32 height = inputImage->GetHeight();
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::R8, width, height);

					const Nz::UInt8* sourcePixels = inputImage->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedImage.GetPixels();
					Nz::UInt8 bpp = Nz::PixelFormatInfo::GetBytesPerPixel(inputImage->GetFormat());

					Nz::UInt32 channelR = Nz::UnderlyingCast(m_channelSources[0]);

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							*cookedPixels++ = sourcePixels[channelR];
							sourcePixels += bpp;
						}
					}
					break;
				}

				case TextureType::BiGreyscale:
				{
					if (noSwizzling)
					{
						// Image is already good
						cookedImage = std::move(*inputImage);
						inputImage.reset();
						break;
					}

					// Apply swizzling
					Nz::UInt32 width = inputImage->GetWidth();
					Nz::UInt32 height = inputImage->GetHeight();
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::RG8, width, height);

					const Nz::UInt8* sourcePixels = inputImage->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedImage.GetPixels();
					Nz::UInt8 bpp = Nz::PixelFormatInfo::GetBytesPerPixel(inputImage->GetFormat());

					Nz::UInt32 channelR = Nz::UnderlyingCast(m_channelSources[0]);
					Nz::UInt32 channelG = Nz::UnderlyingCast(m_channelSources[1]);

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							cookedPixels[0] = sourcePixels[channelR];
							cookedPixels[1] = sourcePixels[channelG];

							cookedPixels += 2;
							sourcePixels += bpp;
						}
					}
					break;
				}

				case TextureType::Normal:
				{
					Nz::UInt32 width = inputImage->GetWidth();
					Nz::UInt32 height = inputImage->GetHeight();
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::RG8, width, height);

					const Nz::UInt8* sourcePixels = inputImage->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedImage.GetPixels();
					Nz::UInt8 bpp = Nz::PixelFormatInfo::GetBytesPerPixel(inputImage->GetFormat());

					Nz::UInt32 channel0 = Nz::UnderlyingCast(m_channelSources[0]);
					Nz::UInt32 channel1 = Nz::UnderlyingCast(m_channelSources[1]);
					Nz::UInt32 channel2 = Nz::UnderlyingCast(m_channelSources[2]);

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							if (sourcePixels[channel2] < 127)
								spdlog::warn("normal map pixel at ({};{}) has Z value < 127: {}", x, y, sourcePixels[2]);

							cookedPixels[0] = sourcePixels[channel0];
							cookedPixels[1] = sourcePixels[channel1];

							sourcePixels += bpp;
							cookedPixels += 2;
						}
					}
					break;
				}
			}

			if (m_generateMipmaps)
			{
				if (!cookedImage.GenerateMipmaps())
				{
					spdlog::error("failed to generate mipmaps for {}", Nz::PathToString(m_outputFile));
					return;
				}
			}

			switch (m_textureType)
			{
				case TextureType::Color:
					if (cookedImage.HasAlpha())
						cookedImage = Nz::ImageCompressor::RGBA8ToBC3(cookedImage);
					else
						cookedImage = Nz::ImageCompressor::RGBA8ToBC1(cookedImage);

					break;

				case TextureType::Greyscale:
					cookedImage = Nz::ImageCompressor::R8ToBC4(cookedImage);
					break;

				case TextureType::BiGreyscale:
				case TextureType::Normal:
					cookedImage = Nz::ImageCompressor::RG8ToBC5(cookedImage);
					break;
			}

			if (!cookedImage.SaveToFile(m_outputFile))
			{
				spdlog::error("failed to save file to {}", Nz::PathToString(m_outputFile));
				return;
			}
		});
	}

	auto TextureCooker::GetInputFiles() const -> InputFileList
	{
		return { m_inputFile };
	}

	auto TextureCooker::GetOutputFiles() const -> OutputFileList
	{
		return { m_outputFile };
	}
}
