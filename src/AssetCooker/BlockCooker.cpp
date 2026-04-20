// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <AssetCooker/BlockCooker.hpp>
#include <CommonLib/BlockLibrary.hpp>
#include <CommonLib/CookedBlockRegistry.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Core/ImageCompressor.hpp>
#include <Nazara/Core/TaskScheduler.hpp>
#include <NazaraUtils/PathUtils.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

namespace tsom
{
	BlockCooker::BlockCooker(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir, const nlohmann::json& doc) :
	m_outputDir(outputDir / Nz::Utf8Path(doc.at("output").get<std::string>())),
	m_textureSize(doc.at("size"))
	{
		std::filesystem::path inputFile = inputDir / Nz::Utf8Path(doc.at("input").get<std::string>());

		std::optional<std::vector<Nz::UInt8>> inputContent = Nz::File::ReadWhole(inputFile);
		if (!inputContent)
			throw std::runtime_error(fmt::format("failed to read {}", Nz::PathToString(inputFile)));

		if (!m_blockLibrary.LoadFromString(std::string_view(reinterpret_cast<const char*>(inputContent->data()), inputContent->size())))
			throw std::runtime_error(fmt::format("failed to load block library", Nz::PathToString(inputFile)));

		const auto& blocks = m_blockLibrary.GetBlocks();
		m_inputs.reserve(blocks.size());

		std::string textureInputDir = Nz::PathToString(inputFile.parent_path());
		for (const BlockLibrary::BlockData& blockData : blocks)
		{
			auto& inputData = m_inputs.emplace_back();
			inputData.files[TextureType::BaseColor] = Nz::Utf8Path(fmt::format("{}/{}.png", textureInputDir, blockData.basePath));
			inputData.files[TextureType::AmbientOcclusion] = Nz::Utf8Path(fmt::format("{}/{}_ao.png", textureInputDir, blockData.basePath));
			inputData.files[TextureType::Height] = Nz::Utf8Path(fmt::format("{}/{}_height.png", textureInputDir, blockData.basePath));
			inputData.files[TextureType::Metalness] = Nz::Utf8Path(fmt::format("{}/{}_metallic.png", textureInputDir, blockData.basePath));
			inputData.files[TextureType::Normal] = Nz::Utf8Path(fmt::format("{}/{}_normal.png", textureInputDir, blockData.basePath));
			inputData.files[TextureType::Roughness] = Nz::Utf8Path(fmt::format("{}/{}_roughness.png", textureInputDir, blockData.basePath));

			for (auto&& [textureType, texturePath] : inputData.files.iter_kv())
			{
				if (std::filesystem::is_regular_file(texturePath))
					m_inputFiles.push_back(texturePath);
				else
					texturePath.clear();
			}

			if (!inputData.files[TextureType::BaseColor].empty())
				m_outputFiles.push_back(m_outputDir / Nz::Utf8Path(fmt::format("{}_color.dds", blockData.name)));

			if (!inputData.files[TextureType::Normal].empty())
				m_outputFiles.push_back(m_outputDir / Nz::Utf8Path(fmt::format("{}_normal.dds", blockData.name)));

			if (!inputData.files[TextureType::Roughness].empty() && !inputData.files[TextureType::Metalness].empty())
				m_outputFiles.push_back(m_outputDir / Nz::Utf8Path(fmt::format("{}_roughness_metalness.dds", blockData.name)));
			else if (!inputData.files[TextureType::Roughness].empty())
				m_outputFiles.push_back(m_outputDir / Nz::Utf8Path(fmt::format("{}_roughness.dds", blockData.name)));

			if (!inputData.files[TextureType::AmbientOcclusion].empty())
				m_outputFiles.push_back(m_outputDir / Nz::Utf8Path(fmt::format("{}_ao.dds", blockData.name)));
		}
	}

	void BlockCooker::Cook(Nz::TaskScheduler& taskScheduler)
	{
		// TODO: Split in more subtasks
		taskScheduler.AddTask([this]
		{
			if (!std::filesystem::is_directory(m_outputDir))
				std::filesystem::create_directories(m_outputDir);

			CookedBlockRegistry registry;

			const auto& blocks = m_blockLibrary.GetBlocks();
			for (std::size_t blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
			{
				const auto& inputData = m_inputs[blockIndex];
				const auto& blockData = blocks[blockIndex];

				CookedBlockRegistry::BlockEntry blockEntry;

				// Handle color map
				if (!inputData.files[TextureType::BaseColor].empty())
				{
					std::shared_ptr<Nz::Image> baseColor = Nz::Image::LoadFromFile(inputData.files[TextureType::BaseColor]);
					if (!baseColor)
					{
						spdlog::error("failed to load {} base color", blockData.name);
						return;
					}

					if (baseColor->GetFormat() != Nz::PixelFormat::RGB8 && baseColor->GetFormat() != Nz::PixelFormat::RGBA8)
					{
						spdlog::error("{} color map is not RGB8 nor RGBA8 (got {})", blockData.name, Nz::PixelFormatInfo::GetName(baseColor->GetFormat()));
						return;
					}

					blockEntry.baseColorFallback = Nz::Color::sRGBToLinear(baseColor->ComputeAverageColor());
					spdlog::debug("{} base color map average color: {};{};{};{}", blockData.name, blockEntry.baseColorFallback.r, blockEntry.baseColorFallback.g, blockEntry.baseColorFallback.b, blockEntry.baseColorFallback.a);

					bool hasAlpha = baseColor->HasAlpha(); // test before potential resize (faster)

					if (baseColor->GetSize() != Nz::Vector3ui32(m_textureSize, m_textureSize, 1))
					{
						spdlog::warn("{} base color has an unexpected size", blockData.name);
						baseColor->Resize(m_textureSize, m_textureSize);
					}

					baseColor->GenerateMipmaps();

					std::string colorFilename = fmt::format("{}_color.dds", blockData.name);

					Nz::Image compressedBaseColor;
					if (hasAlpha)
					{
						// Compress using BC3
						// TODO: Detect 1bit alpha
						spdlog::debug("{} base color map has alpha", blockData.name);

						compressedBaseColor = Nz::ImageCompressor::RGBA8ToBC3(*baseColor);
						blockEntry.baseColorTexture = { CookedBlockRegistry::TextureType::BC3, colorFilename };
					}
					else
					{
						// Compress using BC1
						spdlog::debug("{} base color map has no alpha", blockData.name);

						if (baseColor->GetFormat() == Nz::PixelFormat::RGBA8)
							compressedBaseColor = Nz::ImageCompressor::RGBA8ToBC1(*baseColor);
						else
							compressedBaseColor = Nz::ImageCompressor::RGB8ToBC1(*baseColor);

						blockEntry.baseColorTexture = { CookedBlockRegistry::TextureType::BC1, colorFilename };
					}

					std::filesystem::path targetBaseColorPath = m_outputDir / Nz::Utf8Path(colorFilename);
					if (!compressedBaseColor.SaveToFile(targetBaseColorPath))
					{
						spdlog::error("failed to save file {}", Nz::PathToString(targetBaseColorPath));
						return;
					}
				}

				// Handle normal maps
				if (!inputData.files[TextureType::Normal].empty())
				{
					std::shared_ptr<Nz::Image> normalMap = Nz::Image::LoadFromFile(inputData.files[TextureType::Normal]);
					if (!normalMap)
					{
						spdlog::error("failed to load {} normal map", blockData.name);
						return;
					}

					if (normalMap->GetFormat() != Nz::PixelFormat::RGB8 && normalMap->GetFormat() != Nz::PixelFormat::RGBA8)
					{
						spdlog::error("{} normal map is not RGB8 nor RGBA8 (got {})", blockData.name, Nz::PixelFormatInfo::GetName(normalMap->GetFormat()));
						return;
					}

					if (normalMap->GetSize() != Nz::Vector3ui32(m_textureSize, m_textureSize, 1))
					{
						spdlog::error("{} normal map has an unexpected size", blockData.name);
						return;
					}

					Nz::Image cookedNormalMap(Nz::ImageType::E2D, Nz::PixelFormat::RG8, m_textureSize, m_textureSize);

					const Nz::UInt8* sourcePixels = normalMap->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedNormalMap.GetPixels();
					Nz::UInt8 bpp = Nz::PixelFormatInfo::GetBytesPerPixel(normalMap->GetFormat());

					for (std::size_t y = 0; y < m_textureSize; ++y)
					{
						for (std::size_t x = 0; x < m_textureSize; ++x)
						{
							if (sourcePixels[2] < 127)
								spdlog::debug("{} normal map {};{} has Z value < 127: {}", blockData.name, x, y, sourcePixels[2]);

							cookedPixels[0] = sourcePixels[0];
							cookedPixels[1] = sourcePixels[1];

							sourcePixels += bpp;
							cookedPixels += 2;
						}
					}
					cookedNormalMap.GenerateMipmaps();

					cookedNormalMap = Nz::ImageCompressor::RG8ToBC5(cookedNormalMap);

					std::string normalFilename = fmt::format("{}_normal.dds", blockData.name);
					blockEntry.normalMapTexture = { CookedBlockRegistry::TextureType::BC5, normalFilename };

					std::filesystem::path targetPath = m_outputDir / Nz::Utf8Path(normalFilename);
					if (!cookedNormalMap.SaveToFile(targetPath))
					{
						spdlog::error("failed to save file {}", Nz::PathToString(targetPath));
						return;
					}
				}

				// Roughness/metalness
				blockEntry.metalnessFallback = blockData.metalness;
				blockEntry.roughnessFallback = blockData.roughness;

				if (!inputData.files[TextureType::Roughness].empty())
				{
					std::shared_ptr<Nz::Image> roughnessMap = Nz::Image::LoadFromFile(inputData.files[TextureType::Roughness], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::L8 });
					if (!roughnessMap)
					{
						spdlog::error("failed to load {} roughness map", blockData.name);
						return;
					}

					if (roughnessMap->GetSize() != Nz::Vector3ui32(m_textureSize, m_textureSize, 1))
					{
						spdlog::error("{} roughness map has an unexpected size", blockData.name);
						return;
					}

					blockEntry.roughnessFallback = roughnessMap->ComputeAverageColor().r;

					if (!inputData.files[TextureType::Metalness].empty())
					{
						// BC5 roughness/metalness
						std::shared_ptr<Nz::Image> metalnessMap = Nz::Image::LoadFromFile(inputData.files[TextureType::Metalness], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::L8 });
						if (!metalnessMap)
						{
							spdlog::error("failed to load {} metalness map", blockData.name);
							return;
						}

						if (metalnessMap->GetSize() != Nz::Vector3ui32(m_textureSize, m_textureSize, 1))
						{
							spdlog::error("{} metalness map has an unexpected size", blockData.name);
							return;
						}

						blockEntry.metalnessFallback = metalnessMap->ComputeAverageColor().r;

						Nz::Image cookedRoughnessMetalnessMap(Nz::ImageType::E2D, Nz::PixelFormat::RG8, m_textureSize, m_textureSize);

						const Nz::UInt8* roughnessPixels = roughnessMap->GetConstPixels();
						const Nz::UInt8* metalnessPixels = metalnessMap->GetConstPixels();
						Nz::UInt8* cookedPixels = cookedRoughnessMetalnessMap.GetPixels();
						for (std::size_t y = 0; y < m_textureSize; ++y)
						{
							for (std::size_t x = 0; x < m_textureSize; ++x)
							{
								cookedPixels[0] = *roughnessPixels++;
								cookedPixels[1] = *metalnessPixels++;

								cookedPixels += 2;
							}
						}
						cookedRoughnessMetalnessMap.GenerateMipmaps();

						cookedRoughnessMetalnessMap = Nz::ImageCompressor::RG8ToBC5(cookedRoughnessMetalnessMap);

						std::string roughnessMetalnessFilename = fmt::format("{}_roughness_metalness.dds", blockData.name);
						blockEntry.roughnessMetalnessTexture = { CookedBlockRegistry::TextureType::BC5, roughnessMetalnessFilename };

						std::filesystem::path targetPath = m_outputDir / Nz::Utf8Path(roughnessMetalnessFilename);
						if (!cookedRoughnessMetalnessMap.SaveToFile(targetPath))
						{
							spdlog::error("failed to save file {}", Nz::PathToString(targetPath));
							return;
						}
					}
					else
					{
						// BC4 roughness
						Nz::Image cookedRoughnessMap(Nz::ImageType::E2D, Nz::PixelFormat::R8, m_textureSize, m_textureSize);

						const Nz::UInt8* sourcePixels = roughnessMap->GetConstPixels();
						Nz::UInt8* cookedPixels = cookedRoughnessMap.GetPixels();
						for (std::size_t y = 0; y < m_textureSize; ++y)
						{
							for (std::size_t x = 0; x < m_textureSize; ++x)
								*cookedPixels++ = *sourcePixels++;
						}
						cookedRoughnessMap.GenerateMipmaps();

						cookedRoughnessMap = Nz::ImageCompressor::R8ToBC4(cookedRoughnessMap);

						std::string roughnessMetalnessFilename = fmt::format("{}_roughness.dds", blockData.name);
						blockEntry.roughnessMetalnessTexture = { CookedBlockRegistry::TextureType::BC4, roughnessMetalnessFilename };

						std::filesystem::path targetPath = m_outputDir / Nz::Utf8Path(roughnessMetalnessFilename);
						if (!cookedRoughnessMap.SaveToFile(targetPath))
						{
							spdlog::error("failed to save file {}", Nz::PathToString(targetPath));
							return;
						}
					}
				}
				else if (!inputData.files[TextureType::Metalness].empty())
				{
					spdlog::warn("{} block has no roughness map but has a metalness map, this is unexpected, no roughness-metalness map will be cooked", blockData.name);
				}

				// Ambient Occlusion (+ Heightmap once used)
				blockEntry.ambientOcclusionFallback = 1.0f;

				if (!inputData.files[TextureType::AmbientOcclusion].empty())
				{
					// BC4
					std::shared_ptr<Nz::Image> aoMap = Nz::Image::LoadFromFile(inputData.files[TextureType::AmbientOcclusion], Nz::ImageParams{ .loadFormat = Nz::PixelFormat::L8 });
					if (!aoMap)
					{
						spdlog::error("failed to load {} ambient occlusion map", blockData.name);
						return;
					}

					if (aoMap->GetSize() != Nz::Vector3ui32(m_textureSize, m_textureSize, 1))
					{
						spdlog::error("{} ambient occlusion map has an unexpected size", blockData.name);
						return;
					}

					blockEntry.ambientOcclusionFallback = aoMap->ComputeAverageColor().r;

					Nz::Image cookedAOMap(Nz::ImageType::E2D, Nz::PixelFormat::R8, m_textureSize, m_textureSize);

					const Nz::UInt8* aoPixels = aoMap->GetConstPixels();
					Nz::UInt8* cookedPixels = cookedAOMap.GetPixels();
					for (std::size_t y = 0; y < m_textureSize; ++y)
					{
						for (std::size_t x = 0; x < m_textureSize; ++x)
							*cookedPixels++ = *aoPixels++;
					}

					cookedAOMap.GenerateMipmaps();

					cookedAOMap = Nz::ImageCompressor::R8ToBC4(cookedAOMap);

					std::string aoHeightFilename = fmt::format("{}_ao.dds", blockData.name);
					blockEntry.ambientOcclusionHeightTexture = { CookedBlockRegistry::TextureType::BC4, aoHeightFilename };

					std::filesystem::path targetPath = m_outputDir / Nz::Utf8Path(aoHeightFilename);
					if (!cookedAOMap.SaveToFile(targetPath))
					{
						spdlog::error("failed to save file {}", Nz::PathToString(targetPath));
						return;
					}
				}

				registry.AddBlock(blockData.name, std::move(blockEntry));
			}

			std::filesystem::path registryPath = m_outputDir / Nz::Utf8Path("registry.json");
			if (!registry.SaveToFile(registryPath))
			{
				spdlog::error("failed to save file {}", Nz::PathToString(registryPath));
				return;
			}
		});
	}

	auto BlockCooker::GetInputFiles() const -> InputFileList
	{
		return m_inputFiles;
	}

	auto BlockCooker::GetOutputFiles() const -> OutputFileList
	{
		return m_outputFiles;
	}

}
