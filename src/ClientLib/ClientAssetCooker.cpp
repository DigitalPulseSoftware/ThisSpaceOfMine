// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientAssetCooker.hpp>
#include <ClientLib/ClientAssetCookRegistry.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Core/ImageCompressor.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace tsom
{
	namespace
	{
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
	}

	Nz::Result<void, std::string> ClientAssetCooker::Cook(ClientBlockLibrary& blockLibrary)
	{
		auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();

		const auto& blocks = blockLibrary.GetBlocks();

		std::filesystem::path cacheDir = Nz::Utf8Path("cache");
		std::filesystem::path cookedAssetsDir = Nz::Utf8Path("CookedAssets");
		std::filesystem::path blockDir = Nz::Utf8Path("Blocks");

		std::filesystem::path cookedAssetsPath = cacheDir / cookedAssetsDir;
		std::filesystem::path cookedBlockPath = cookedAssetsPath / blockDir;
		if (!std::filesystem::is_directory(cookedBlockPath))
			std::filesystem::create_directories(cookedBlockPath);

		constexpr Nz::UInt32 imageSize = 2048;

		ClientAssetCookRegistry registry;

		for (const ClientBlockLibrary::BlockData& blockData : blocks)
		{
			ClientAssetCookRegistry::BlockEntry blockEntry;

			Nz::EnumArray<TextureType, std::shared_ptr<Nz::Stream>> streams;
			streams[TextureType::BaseColor] = fs.GetFile(fmt::format("assets/{}.png", blockData.basePath));
			streams[TextureType::AmbientOcclusion] = fs.GetFile(fmt::format("assets/{}_ao.png", blockData.basePath));
			streams[TextureType::Height] = fs.GetFile(fmt::format("assets/{}_height.png", blockData.basePath));
			streams[TextureType::Metalness] = fs.GetFile(fmt::format("assets/{}_metallic.png", blockData.basePath));
			streams[TextureType::Normal] = fs.GetFile(fmt::format("assets/{}_normal.png", blockData.basePath));
			streams[TextureType::Roughness] = fs.GetFile(fmt::format("assets/{}_roughness.png", blockData.basePath));

			// Handle color map
			if (streams[TextureType::BaseColor])
			{
				if (!streams[TextureType::BaseColor])
					return Nz::Err(fmt::format("failed to open {} base color", blockData.name));

				std::shared_ptr<Nz::Image> baseColor = Nz::Image::LoadFromStream(*streams[TextureType::BaseColor], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::RGBA8 });
				if (!baseColor)
					return Nz::Err(fmt::format("failed to load {} base color", blockData.name));

				blockEntry.baseColorFallback = baseColor->ComputeAverageColor();
				spdlog::debug("{} base color map average color: {};{};{};{}", blockData.name, blockEntry.baseColorFallback.r, blockEntry.baseColorFallback.g, blockEntry.baseColorFallback.b, blockEntry.baseColorFallback.a);

				if (baseColor->GetSize() != Nz::Vector3ui32(imageSize, imageSize, 1))
				{
					spdlog::warn("{} base color has an unexpected size", blockData.name);
					baseColor->Resize(imageSize, imageSize);
				}

				baseColor->GenerateMipmaps();

				std::filesystem::path colorFilename = Nz::Utf8Path(fmt::format("{}_color.dds", blockData.name));

				Nz::Image compressedBaseColor;
				if (baseColor->HasAlpha())
				{
					// Compress using BC3
					// TODO: Detect 1bit alpha
					spdlog::debug("{} base color map has alpha", blockData.name);

					compressedBaseColor = Nz::ImageCompressor::RGBA8ToBC3(*baseColor);
					blockEntry.baseColorTexture = { ClientAssetCookRegistry::TextureType::BC3, Nz::PathToString(blockDir / colorFilename) };
				}
				else
				{
					// Compress using BC1
					spdlog::debug("{} base color map has no alpha", blockData.name);

					compressedBaseColor = Nz::ImageCompressor::RGBA8ToBC1(*baseColor);
					blockEntry.baseColorTexture = { ClientAssetCookRegistry::TextureType::BC1, Nz::PathToString(blockDir / colorFilename) };
				}

				std::filesystem::path targetBaseColorPath = cookedBlockPath / Nz::Utf8Path(fmt::format("{}_color.dds", blockData.name));
				if (!compressedBaseColor.SaveToFile(cookedBlockPath / colorFilename))
					return Nz::Err(fmt::format("failed to save file {}", Nz::PathToString(targetBaseColorPath)));
			}

			// Handle normal maps
			if (streams[TextureType::Normal])
			{
				std::shared_ptr<Nz::Image> normalMap = Nz::Image::LoadFromStream(*streams[TextureType::Normal], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::RGBA8 });
				if (!normalMap)
					return Nz::Err(fmt::format("failed to load {} normal map", blockData.name));

				if (normalMap->GetSize() != Nz::Vector3ui32(imageSize, imageSize, 1))
					return Nz::Err(fmt::format("{} normal map has an unexpected size", blockData.name));

				Nz::Image cookedNormalMap(Nz::ImageType::E2D, Nz::PixelFormat::RG8, imageSize, imageSize);

				const Nz::UInt8* sourcePixels = normalMap->GetConstPixels();
				Nz::UInt8* cookedPixels = cookedNormalMap.GetPixels();
				for (std::size_t y = 0; y < imageSize; ++y)
				{
					for (std::size_t x = 0; x < imageSize; ++x)
					{
						if (sourcePixels[2] < 127)
							spdlog::debug("{} normal map {};{} has Z value < 127: {}", blockData.name, x, y, sourcePixels[2]);

						cookedPixels[0] = sourcePixels[0];
						cookedPixels[1] = sourcePixels[1];

						sourcePixels += 4;
						cookedPixels += 2;
					}
				}
				cookedNormalMap.GenerateMipmaps();

				cookedNormalMap = Nz::ImageCompressor::RG8ToBC5(cookedNormalMap);

				std::filesystem::path normalFilename = Nz::Utf8Path(fmt::format("{}_normal.dds", blockData.name));
				blockEntry.normalMapTexture = { ClientAssetCookRegistry::TextureType::BC5, Nz::PathToString(blockDir / normalFilename) };

				std::filesystem::path targetPath = cookedBlockPath / normalFilename;
				if (!cookedNormalMap.SaveToFile(targetPath))
					return Nz::Err(fmt::format("failed to save file {}", Nz::PathToString(targetPath)));
			}

			// Roughness/metalness
			if (streams[TextureType::Roughness])
			{
				std::shared_ptr<Nz::Image> roughnessMap = Nz::Image::LoadFromStream(*streams[TextureType::Roughness], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::L8 });
				if (!roughnessMap)
					return Nz::Err(fmt::format("failed to load {} roughness map", blockData.name));

				if (roughnessMap->GetSize() != Nz::Vector3ui32(imageSize, imageSize, 1))
					return Nz::Err(fmt::format("{} roughness map has an unexpected size", blockData.name));

				if (streams[TextureType::Metalness])
				{
					// BC5 roughness/metalness
					std::shared_ptr<Nz::Image> metalnessMap = Nz::Image::LoadFromStream(*streams[TextureType::Metalness], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::L8 });
					if (!metalnessMap)
						return Nz::Err(fmt::format("failed to load {} metalness map", blockData.name));

					if (metalnessMap->GetSize() != Nz::Vector3ui32(imageSize, imageSize, 1))
						return Nz::Err(fmt::format("{} metalness map has an unexpected size", blockData.name));

					Nz::Image cookedRoughnessMetalnessMap(Nz::ImageType::E2D, Nz::PixelFormat::RG8, imageSize, imageSize);

					const Nz::UInt8* roughnessPixels = roughnessMap->GetConstPixels();
					const Nz::UInt8* metalnessPixels = metalnessMap->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedRoughnessMetalnessMap.GetPixels();
					for (std::size_t y = 0; y < imageSize; ++y)
					{
						for (std::size_t x = 0; x < imageSize; ++x)
						{
							cookedPixels[0] = *roughnessPixels++;
							cookedPixels[1] = *metalnessPixels++;

							cookedPixels += 2;
						}
					}
					cookedRoughnessMetalnessMap.GenerateMipmaps();

					cookedRoughnessMetalnessMap = Nz::ImageCompressor::RG8ToBC5(cookedRoughnessMetalnessMap);

					std::filesystem::path roughnessMetalnessFilename = Nz::Utf8Path(fmt::format("{}_roughness_metalness.dds", blockData.name));
					std::filesystem::path targetPath = cookedBlockPath / roughnessMetalnessFilename;

					blockEntry.roughnessMetalnessTexture = { ClientAssetCookRegistry::TextureType::BC5, Nz::PathToString(blockDir / roughnessMetalnessFilename) };

					if (!cookedRoughnessMetalnessMap.SaveToFile(targetPath))
						return Nz::Err(fmt::format("failed to save file {}", Nz::PathToString(targetPath)));
				}
				else
				{
					// BC4 roughness
					Nz::Image cookedRoughnessMap(Nz::ImageType::E2D, Nz::PixelFormat::R8, imageSize, imageSize);

					const Nz::UInt8* sourcePixels = roughnessMap->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedRoughnessMap.GetPixels();
					for (std::size_t y = 0; y < imageSize; ++y)
					{
						for (std::size_t x = 0; x < imageSize; ++x)
							*cookedPixels++ = *sourcePixels++;
					}
					cookedRoughnessMap.GenerateMipmaps();

					cookedRoughnessMap = Nz::ImageCompressor::R8ToBC4(cookedRoughnessMap);

					std::filesystem::path roughnessMetalnessFilename = Nz::Utf8Path(fmt::format("{}_roughness.dds", blockData.name));
					std::filesystem::path targetPath = cookedBlockPath / roughnessMetalnessFilename;

					blockEntry.roughnessMetalnessTexture = { ClientAssetCookRegistry::TextureType::BC4, Nz::PathToString(blockDir / roughnessMetalnessFilename) };

					if (!cookedRoughnessMap.SaveToFile(targetPath))
						return Nz::Err(fmt::format("failed to save file {}", Nz::PathToString(targetPath)));
				}
			}
			else if (streams[TextureType::Metalness])
			{
				spdlog::warn("{} block has no roughness map but has a metalness map, this is unexpected, no roughness-metalness map will be cooked");
			}

			// Ambient Occlusion (+ Heightmap once used)
			if (streams[TextureType::AmbientOcclusion])
			{
				// BC4
				std::shared_ptr<Nz::Image> aoMap = Nz::Image::LoadFromStream(*streams[TextureType::AmbientOcclusion], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::L8 });
				if (!aoMap)
					return Nz::Err(fmt::format("failed to load {} ambient occlusion map", blockData.name));

				if (aoMap->GetSize() != Nz::Vector3ui32(imageSize, imageSize, 1))
					return Nz::Err(fmt::format("{} ambient occlusion map has an unexpected size", blockData.name));

				Nz::Image cookedAOMap(Nz::ImageType::E2D, Nz::PixelFormat::R8, imageSize, imageSize);

				const Nz::UInt8* aoPixels = aoMap->GetConstPixels();
				Nz::UInt8* cookedPixels = cookedAOMap.GetPixels();
				for (std::size_t y = 0; y < imageSize; ++y)
				{
					for (std::size_t x = 0; x < imageSize; ++x)
						*cookedPixels++ = *aoPixels++;
				}

				cookedAOMap.GenerateMipmaps();

				cookedAOMap = Nz::ImageCompressor::R8ToBC4(cookedAOMap);

				std::filesystem::path aoHeightFilename = Nz::Utf8Path(fmt::format("{}_ao.dds", blockData.name));
				blockEntry.ambientOcclusionHeightTexture = { ClientAssetCookRegistry::TextureType::BC4, Nz::PathToString(blockDir / aoHeightFilename) };

				std::filesystem::path targetPath = cookedBlockPath / aoHeightFilename;
				if (!cookedAOMap.SaveToFile(targetPath))
					return Nz::Err(fmt::format("failed to save file {}", Nz::PathToString(targetPath)));
			}

			registry.AddBlock(blockData.name, std::move(blockEntry));
		}

		std::filesystem::path registryPath = cookedAssetsPath / Nz::Utf8Path("registry.json");
		if (!registry.SaveToFile(registryPath))
			return Nz::Err(fmt::format("failed to save file {}", Nz::PathToString(registryPath)));

		return Nz::Ok();
	}
}
