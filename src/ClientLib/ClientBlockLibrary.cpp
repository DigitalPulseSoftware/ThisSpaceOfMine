// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientBlockLibrary.hpp>
#include <NazaraUtils/EnumArray.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Renderer/Texture.hpp>
#include <NZSL/Math/FieldOffsets.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	namespace
	{
		enum class TextureType
		{
			AmbientOcclusion_Height,
			BaseColor,
			Normal,
			Roughness_Metalness,

			Max = Roughness_Metalness
		};

		struct GlobalBlockBufferEntryOffsets
		{
			nzsl::FieldOffsets fieldOffsets;
			std::size_t baseColorFallback;
			std::size_t ambientOcclusionHeightMapIndices;
			std::size_t baseColorMapIndices;
			std::size_t metalness;
			std::size_t normalMapIndices;
			std::size_t ambientOcclusion;
			std::size_t roughness;
			std::size_t roughnessMetalnessMapIndices;
		};

		struct GlobalBlockBufferOffsets
		{
			nzsl::FieldOffsets fieldOffsets;
			std::size_t entries;
		};

		constexpr GlobalBlockBufferEntryOffsets BuildGlobalBlockBufferEntryOffsets()
		{
			GlobalBlockBufferEntryOffsets bufferOffsets {
				nzsl::FieldOffsets(nzsl::StructLayout::Std430)
			};

			bufferOffsets.baseColorFallback = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float4);
			bufferOffsets.baseColorMapIndices = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.normalMapIndices = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.ambientOcclusionHeightMapIndices = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.roughnessMetalnessMapIndices = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.ambientOcclusion = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.metalness = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.roughness = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);

			return bufferOffsets;
		}

		constexpr GlobalBlockBufferEntryOffsets s_blockBufferEntryOffsets = BuildGlobalBlockBufferEntryOffsets();

		constexpr GlobalBlockBufferOffsets BuildGlobalBlockBufferOffsets()
		{
			GlobalBlockBufferOffsets bufferOffsets{
				nzsl::FieldOffsets(nzsl::StructLayout::Std430)
			};

			bufferOffsets.entries = bufferOffsets.fieldOffsets.AddStructArray(s_blockBufferEntryOffsets.fieldOffsets, 1);

			return bufferOffsets;
		}

		constexpr GlobalBlockBufferOffsets s_blockBufferOffsets = BuildGlobalBlockBufferOffsets();
	}

	void ClientBlockLibrary::BuildTexture(Nz::RenderDevice& renderDevice)
	{
		std::size_t bufferSize = s_blockBufferOffsets.fieldOffsets.GetAlignedSize() * m_blocks.size();

		m_globalBlockBuffer = renderDevice.InstantiateBuffer(bufferSize, Nz::BufferUsage::DeviceLocal | Nz::BufferUsage::StorageBuffer | Nz::BufferUsage::PersistentMapping);
		m_globalBlockBufferPtr = m_globalBlockBuffer->Map(0, bufferSize);

		auto& fs = m_applicationBase.GetComponent<Nz::FilesystemAppComponent>();

		std::optional<ClientAssetCookRegistry> cookRegistry;
		fs.GetFileContent("CookedAssets/registry.json", [&](const void* ptr, Nz::UInt64 size)
		{
			cookRegistry = ClientAssetCookRegistry::LoadFromContent(std::string_view(reinterpret_cast<const char*>(ptr), Nz::SafeCast<std::size_t>(size)));
			return true;
		});

		if (!cookRegistry)
			throw std::runtime_error("failed to load cook registry");

		struct BlockTexture
		{
			Nz::EnumArray<TextureType, const ClientAssetCookRegistry::Texture*> textureData;
		};

		Nz::UInt8* blockBufferPtr = static_cast<Nz::UInt8*>(m_globalBlockBufferPtr) + s_blockBufferOffsets.entries;

		Nz::EnumArray<ClientAssetCookRegistry::TextureType, Nz::UInt32> textureCount;
		textureCount.fill(0);

		Nz::UInt32 sliceCount = 0;
		std::vector<BlockTexture> remainingBlockTextures;
		remainingBlockTextures.reserve(m_blocks.size());

		for (const BlockData& blockData : m_blocks)
		{
			const auto& cookedBlockData = cookRegistry->GetBlock(blockData.name);

			std::size_t blockIndex = remainingBlockTextures.size();
			auto& blockTexture = remainingBlockTextures.emplace_back();
			blockTexture.textureData[TextureType::AmbientOcclusion_Height] = &cookedBlockData.ambientOcclusionHeightTexture;
			blockTexture.textureData[TextureType::BaseColor] = &cookedBlockData.baseColorTexture;
			blockTexture.textureData[TextureType::Normal] = &cookedBlockData.normalMapTexture;
			blockTexture.textureData[TextureType::Roughness_Metalness] = &cookedBlockData.roughnessMetalnessTexture;

			Nz::UInt8* blockDataPtr = blockBufferPtr + blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize();
			Nz::AccessByOffset<Nz::Color&>(blockDataPtr, s_blockBufferEntryOffsets.baseColorFallback) = cookedBlockData.baseColorFallback;
			Nz::AccessByOffset<Nz::Vector2i32&>(blockDataPtr, s_blockBufferEntryOffsets.ambientOcclusionHeightMapIndices) = { -1, -1 };
			Nz::AccessByOffset<Nz::Vector2i32&>(blockDataPtr, s_blockBufferEntryOffsets.baseColorMapIndices) = { -1, -1 };
			Nz::AccessByOffset<Nz::Vector2i32&>(blockDataPtr, s_blockBufferEntryOffsets.normalMapIndices) = { -1, -1 };
			Nz::AccessByOffset<Nz::Vector2i32&>(blockDataPtr, s_blockBufferEntryOffsets.roughnessMetalnessMapIndices) = { -1, -1 };
			Nz::AccessByOffset<float&>(blockDataPtr, s_blockBufferEntryOffsets.ambientOcclusion) = cookedBlockData.ambientOcclusionFallback;
			Nz::AccessByOffset<float&>(blockDataPtr, s_blockBufferEntryOffsets.metalness) = cookedBlockData.metalnessFallback;
			Nz::AccessByOffset<float&>(blockDataPtr, s_blockBufferEntryOffsets.roughness) = cookedBlockData.roughnessFallback;

			for (const auto& [stream, textureData] : blockTexture.textureData.iter_kv())
			{
				if (textureData->type != ClientAssetCookRegistry::TextureType::None)
					textureCount[textureData->type]++;
			}
		}

		constexpr std::size_t texSize = 2048; // TODO: use texture size?

		// BC1/BC3 sRGB formats are not well supported with OpenGL, sRGB to linear conversion is done in shader
		// TODO: Add a shader option to use sRGB formats if supported to avoid conversion cost
		constexpr Nz::EnumArray<ClientAssetCookRegistry::TextureType, Nz::PixelFormat> textureFormat = {
			Nz::PixelFormat::BC1_RGBA_Unorm, 
			Nz::PixelFormat::BC3_Unorm,
			Nz::PixelFormat::BC4_Unorm,
			Nz::PixelFormat::BC5_Unorm  // since BC5 is used for normal maps and roughness/metalness maps we can't use Snorm
		};

		Nz::EnumArray<ClientAssetCookRegistry::TextureType, std::shared_ptr<Nz::Texture>> blockTextures;
		for (auto&& [type, sliceCount] : textureCount.iter_kv())
		{
			if (sliceCount > 0)
			{
				blockTextures[type] = renderDevice.InstantiateTexture({
					.pixelFormat = textureFormat[type],
					.type = Nz::ImageType::E2D_Array,
					.layerCount = sliceCount,
					.height = texSize,
					.width = texSize
				});
			}
		}

		constexpr Nz::EnumArray<TextureType, std::size_t> textureSliceOffsets = {
			s_blockBufferEntryOffsets.ambientOcclusionHeightMapIndices,
			s_blockBufferEntryOffsets.baseColorMapIndices,
			s_blockBufferEntryOffsets.normalMapIndices,
			s_blockBufferEntryOffsets.roughnessMetalnessMapIndices
		};

		Nz::EnumArray<ClientAssetCookRegistry::TextureType, Nz::UInt32> textureSlice;
		textureSlice.fill(0);

		std::vector<std::pair<ClientAssetCookRegistry::TextureType, std::uint32_t>> previewTextures(m_blocks.size());

		for (std::size_t blockIndex = 0; blockIndex < remainingBlockTextures.size(); ++blockIndex)
		{
			const BlockTexture& blockTexture = remainingBlockTextures[blockIndex];

			for (const auto& [textureType, textureData] : blockTexture.textureData.iter_kv())
			{
				if (textureData->type == ClientAssetCookRegistry::TextureType::None)
					continue;

				std::string texturePath = fmt::format("CookedAssets/{}", textureData->path);
				std::shared_ptr<Nz::Stream> stream = fs.GetFile(texturePath);
				if (!stream)
				{
					spdlog::error("asset {} not found", texturePath);
					continue;
				}

				std::shared_ptr<Nz::Image> image = Nz::Image::LoadFromStream(*stream);
				if (!image)
				{
					spdlog::error("failed to load {}", texturePath);
					continue;
				}

				for (Nz::UInt8 level = 0; level < image->GetLevelCount(); ++level)
				{
					blockTextures[textureData->type]->Update([&](void* pixelBuffer, Nz::UInt32 rowPitch, Nz::UInt32 depthPitch)
					{
						NazaraAssert(rowPitch == 0 && depthPitch == 0);
						std::memcpy(pixelBuffer, image->GetConstPixels(level), Nz::PixelFormatInfo::ComputeSize(image->GetFormat(), image->GetWidth(level), image->GetHeight(level), 1));
						return true;
					}, Nz::Boxui(0, 0, textureSlice[textureData->type], image->GetWidth(level), image->GetHeight(level), 1), level);
				}

				Nz::Vector2i32& blockTextureSlice = Nz::AccessByOffset<Nz::Vector2i32&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + textureSliceOffsets[textureType]);
				blockTextureSlice = { static_cast<Nz::Int32>(textureData->type), static_cast<Nz::Int32>(textureSlice[textureData->type]) };

				if (textureType == TextureType::BaseColor)
					previewTextures[blockIndex] = std::make_pair(textureData->type, textureSlice[textureData->type]);

				textureSlice[textureData->type]++;
			}
		}

		for (auto&& [type, texture] : blockTextures.iter_kv())
		{
			if (texture)
				m_blockTextures[type] = Nz::TextureAsset::CreateFromTexture(std::move(texture));
		}

		m_previewTextures.resize(m_blocks.size());
		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			const auto& blockData = m_blocks[blockIndex];

			Nz::TextureViewInfo slotTexView = {
				.viewType = Nz::ImageType::E2D,
				.baseArrayLayer = previewTextures[blockIndex].second
			};

			m_previewTextures[blockIndex] = Nz::TextureAsset::CreateView(m_blockTextures[previewTextures[blockIndex].first], slotTexView);
		}
	}
}
