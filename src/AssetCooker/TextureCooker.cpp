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
	NLOHMANN_JSON_SERIALIZE_ENUM(tsom::TextureCooker::SourceChannel, {
		{tsom::TextureCooker::SourceChannel::Red, "Red"},
		{tsom::TextureCooker::SourceChannel::InvRed, "InvRed"},
		{tsom::TextureCooker::SourceChannel::Green, "Green"},
		{tsom::TextureCooker::SourceChannel::InvGreen, "InvGreen"},
		{tsom::TextureCooker::SourceChannel::Blue, "Blue"},
		{tsom::TextureCooker::SourceChannel::InvBlue, "InvBlue"},
		{tsom::TextureCooker::SourceChannel::Alpha, "Alpha"},
		{tsom::TextureCooker::SourceChannel::InvAlpha, "InvAlpha"},
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
	namespace
	{
		struct TextureChannelData
		{
			Nz::UInt32 channelIndex;
			bool inversed;
		};

		constexpr Nz::EnumArray<TextureCooker::SourceChannel, TextureChannelData> s_textureChannelData = {
			TextureChannelData { 0, false },
			TextureChannelData { 0, true },
			TextureChannelData { 1, false },
			TextureChannelData { 1, true },
			TextureChannelData { 2, false },
			TextureChannelData { 2, true },
			TextureChannelData { 3, false },
			TextureChannelData { 3, true },
		};
	}

	TextureCooker::TextureCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_outputFile(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>())),
	m_compress(doc.value("compress", true)),
	m_generateMipmaps(doc.value("generateMipmaps", true)),
	m_textureType(doc.value("type", TextureType::Color))
	{
		if (auto it = doc.find("inputs"); it != doc.end())
		{
			const nlohmann::json& inputs = *it;
			for (std::size_t i = 0; i < inputs.size(); ++i)
				m_inputFiles[i] = inputDir / Nz::Utf8Path(inputs[i].get<std::string>());
		}
		else
			m_inputFiles[0] = inputDir / Nz::Utf8Path(doc.at("input").get<std::string>());

		m_sourceChannel[0] = doc.value("channel0", SourceChannel::Red);
		m_sourceChannel[1] = doc.value("channel1", SourceChannel::Green);
		m_sourceChannel[2] = doc.value("channel2", SourceChannel::Blue);
		m_sourceChannel[3] = doc.value("channel3", SourceChannel::Alpha);

		m_sourceTexture[0] = doc.value("channelSourceTexture0", 0);
		m_sourceTexture[1] = doc.value("channelSourceTexture1", 0);
		m_sourceTexture[2] = doc.value("channelSourceTexture2", 0);
		m_sourceTexture[3] = doc.value("channelSourceTexture3", 0);
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

			bool noSwizzling =
				s_textureChannelData[m_sourceChannel[0]].channelIndex == 0 &&
				s_textureChannelData[m_sourceChannel[1]].channelIndex == 1 &&
				s_textureChannelData[m_sourceChannel[2]].channelIndex == 2 &&
				s_textureChannelData[m_sourceChannel[3]].channelIndex == 3;

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

			std::array<std::shared_ptr<Nz::Image>, 4> inputImages;
			for (std::size_t i = 0; i < inputImages.size(); ++i)
			{
				if (m_inputFiles[i].empty())
					continue;

				inputImages[i] = Nz::Image::LoadFromFile(m_inputFiles[i], imageParams);
				if (!inputImages[i])
				{
					spdlog::error("failed to load image from {}", Nz::PathToString(m_inputFiles[i]));
					return;
				}
			}

			Nz::UInt32 width = inputImages[0]->GetWidth();
			Nz::UInt32 height = inputImages[0]->GetHeight();
			Nz::UInt8 bpp = Nz::PixelFormatInfo::GetBytesPerPixel(inputImages[0]->GetFormat());

			for (std::size_t i = 0; i < m_sourceTexture.size(); ++i)
			{
				Nz::UInt32 imageIndex = m_sourceTexture[i];
				if (!inputImages[imageIndex])
				{
					spdlog::error("channel references texture #{} that's not valid", imageIndex);
					return;
				}

				if (inputImages[imageIndex]->GetWidth() != width && inputImages[imageIndex]->GetHeight() != height)
				{
					spdlog::error("texture #{} don't match texture #0 dimensions", imageIndex);
					return;
				}

				if (inputImages[imageIndex]->GetFormat() != inputImages[0]->GetFormat())
				{
					spdlog::error("texture #{} don't match texture #0 format", imageIndex);
					return;
				}
			}

			std::array<const Nz::UInt8*, 4> imagePixels;
			for (std::size_t i = 0; i < inputImages.size(); ++i)
				imagePixels[i] = inputImages[i] ? inputImages[i]->GetConstPixels() : nullptr;

			std::array<const Nz::UInt8*, 4> sourcePixels;
			for (std::size_t i = 0; i < sourcePixels.size(); ++i)
				sourcePixels[i] = imagePixels[m_sourceTexture[i]];

			Nz::Image cookedImage;
			switch (m_textureType)
			{
				case TextureType::Color:
				{
					if (noSwizzling && m_sourceTexture[0] == 0 && m_sourceTexture[1] == 0 && m_sourceTexture[2] == 0 && m_sourceTexture[3] == 0)
					{
						// Image is already good
						cookedImage = std::move(*inputImages[0]);
						inputImages[0].reset();
						break;
					}

					// Apply swizzling
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::RGBA8, width, height);

					Nz::UInt8* cookedPixels = cookedImage.GetPixels();

					auto [channelRIndex, channelRInv] = s_textureChannelData[m_sourceChannel[0]];
					auto [channelGIndex, channelGInv] = s_textureChannelData[m_sourceChannel[1]];
					auto [channelBIndex, channelBInv] = s_textureChannelData[m_sourceChannel[2]];
					auto [channelAIndex, channelAInv] = s_textureChannelData[m_sourceChannel[3]];

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							cookedPixels[0] = (channelRInv) ? 0xFF - sourcePixels[0][channelRIndex] : sourcePixels[0][channelRIndex];
							cookedPixels[1] = (channelGInv) ? 0xFF - sourcePixels[1][channelGIndex] : sourcePixels[1][channelGIndex];
							cookedPixels[2] = (channelBInv) ? 0xFF - sourcePixels[2][channelBIndex] : sourcePixels[2][channelBIndex];
							cookedPixels[3] = (channelAInv) ? 0xFF - sourcePixels[3][channelAIndex] : sourcePixels[3][channelAIndex];
							cookedPixels += 4;

							for (std::size_t i = 0; i < sourcePixels.size(); ++i)
								sourcePixels[i] += bpp;
						}
					}

					break;
				}

				case TextureType::Greyscale:
				{
					if (noSwizzling && m_sourceTexture[0] == 0)
					{
						// Image is already good
						cookedImage = std::move(*inputImages[0]);
						inputImages[0].reset();
						break;
					}

					// Apply swizzling
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::R8, width, height);

					Nz::UInt8* cookedPixels = cookedImage.GetPixels();

					auto [channelRIndex, channelRInv] = s_textureChannelData[m_sourceChannel[0]];

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							*cookedPixels++ = (channelRInv) ? 0xFF - sourcePixels[0][channelRIndex] : sourcePixels[0][channelRIndex];

							for (std::size_t i = 0; i < sourcePixels.size(); ++i)
								sourcePixels[i] += bpp;
						}
					}
					break;
				}

				case TextureType::BiGreyscale:
				{
					if (noSwizzling && m_sourceTexture[0] == 0 && m_sourceTexture[1] == 0)
					{
						// Image is already good
						cookedImage = std::move(*inputImages[0]);
						inputImages[0].reset();
						break;
					}

					// Apply swizzling
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::RG8, width, height);

					Nz::UInt8* cookedPixels = cookedImage.GetPixels();

					auto [channelRIndex, channelRInv] = s_textureChannelData[m_sourceChannel[0]];
					auto [channelGIndex, channelGInv] = s_textureChannelData[m_sourceChannel[1]];

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							cookedPixels[0] = (channelRInv) ? 0xFF - sourcePixels[0][channelRIndex] : sourcePixels[0][channelRIndex];
							cookedPixels[1] = (channelGInv) ? 0xFF - sourcePixels[1][channelGIndex] : sourcePixels[1][channelGIndex];
							cookedPixels += 2;

							for (std::size_t i = 0; i < sourcePixels.size(); ++i)
								sourcePixels[i] += bpp;
						}
					}
					break;
				}

				case TextureType::Normal:
				{
					cookedImage.Create(Nz::ImageType::E2D, Nz::PixelFormat::RG8, width, height);

					Nz::UInt8* cookedPixels = cookedImage.GetPixels();

					auto [channel0Index, channel0Inv] = s_textureChannelData[m_sourceChannel[0]];
					auto [channel1Index, channel1Inv] = s_textureChannelData[m_sourceChannel[1]];
					auto [channel2Index, channel2Inv] = s_textureChannelData[m_sourceChannel[2]];

					for (std::size_t y = 0; y < height; ++y)
					{
						for (std::size_t x = 0; x < width; ++x)
						{
							Nz::UInt32 zValue = (channel2Inv) ? 0xFF - sourcePixels[2][channel2Index] : sourcePixels[2][channel2Index];
							if (zValue < 127)
								spdlog::warn("normal map pixel at ({};{}) has Z value < 127: {}", x, y, sourcePixels[2][2]);

							cookedPixels[0] = (channel0Inv) ? 0xFF - sourcePixels[0][channel0Index] : sourcePixels[0][channel0Index];
							cookedPixels[1] = (channel1Inv) ? 0xFF - sourcePixels[1][channel1Index] : sourcePixels[1][channel1Index];
							cookedPixels += 2;

							for (std::size_t i = 0; i < sourcePixels.size(); ++i)
								sourcePixels[i] += bpp;
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
		InputFileList inputFiles { m_inputFiles[0] };
		for (std::size_t i = 1; i < 4; ++i)
		if (!m_inputFiles[i].empty())
			inputFiles.push_back(m_inputFiles[i]);

		return inputFiles;
	}

	auto TextureCooker::GetOutputFiles() const -> OutputFileList
	{
		return { m_outputFile };
	}
}
