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
#include <fmt/format.h>

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
			std::size_t ambientOcclusionHeightMapIndex;
			std::size_t baseColorMapIndex;
			std::size_t metalness;
			std::size_t normalMapIndex;
			std::size_t roughness;
			std::size_t roughnessMetalnessMapIndex;
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
			bufferOffsets.baseColorMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.normalMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.ambientOcclusionHeightMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
			bufferOffsets.roughnessMetalnessMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Int2);
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
		std::size_t bufferSize = s_blockBufferOffsets.fieldOffsets.GetSize() * m_blocks.size();

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
		std::vector<BlockTexture> blockTextures;
		blockTextures.reserve(m_blocks.size());

		for (const BlockData& blockData : m_blocks)
		{
			const auto& cookedBlockData = cookRegistry->GetBlock(blockData.name);

			std::size_t blockIndex = blockTextures.size();
			auto& blockTexture = blockTextures.emplace_back();
			blockTexture.textureData[TextureType::AmbientOcclusion_Height] = &cookedBlockData.ambientOcclusionHeightTexture;
			blockTexture.textureData[TextureType::BaseColor] = &cookedBlockData.baseColorTexture;
			blockTexture.textureData[TextureType::Normal] = &cookedBlockData.normalMapTexture;
			blockTexture.textureData[TextureType::Roughness_Metalness] = &cookedBlockData.roughnessMetalnessTexture;

			Nz::AccessByOffset<Nz::Color&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.baseColorFallback) = cookedBlockData.baseColorFallback;
			Nz::AccessByOffset<Nz::Vector2i32&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.ambientOcclusionHeightMapIndex) = { -1, -1 };
			Nz::AccessByOffset<Nz::Vector2i32&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.baseColorMapIndex) = { -1, -1 };
			Nz::AccessByOffset<Nz::Vector2i32&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.normalMapIndex) = { -1, -1 };
			Nz::AccessByOffset<Nz::Vector2i32&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.roughnessMetalnessMapIndex) = { -1, -1 };
			Nz::AccessByOffset<float&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.metalness) = blockData.metalness;
			Nz::AccessByOffset<float&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.roughness) = blockData.roughness;

			for (const auto& [stream, textureData] : blockTexture.textureData.iter_kv())
			{
				if (textureData->type != ClientAssetCookRegistry::TextureType::None)
					textureCount[textureData->type]++;
			}
		}

		constexpr std::size_t texSize = 2048; // TODO: use texture size?

		constexpr Nz::EnumArray<ClientAssetCookRegistry::TextureType, Nz::PixelFormat> textureFormat = {
			Nz::PixelFormat::BC1_RGBA_Unorm,
			Nz::PixelFormat::BC3_Unorm,
			Nz::PixelFormat::BC4_Unorm,
			Nz::PixelFormat::BC5_Unorm  // since BC5 is used for normal maps and roughness/metalness maps we can't use Snorm
		};

		for (auto&& [type, sliceCount] : textureCount.iter_kv())
		{
			if (sliceCount > 0)
			{
				m_blockTextures[type] = renderDevice.InstantiateTexture({
					.pixelFormat = textureFormat[type],
					.type = Nz::ImageType::E2D_Array,
					.layerCount = sliceCount,
					.height = texSize,
					.width = texSize
				});
			}
		}

		constexpr Nz::EnumArray<TextureType, std::size_t> textureSliceOffsets = {
			s_blockBufferEntryOffsets.ambientOcclusionHeightMapIndex,
			s_blockBufferEntryOffsets.baseColorMapIndex,
			s_blockBufferEntryOffsets.normalMapIndex,
			s_blockBufferEntryOffsets.roughnessMetalnessMapIndex
		};

		Nz::EnumArray<ClientAssetCookRegistry::TextureType, Nz::UInt32> textureSlice;
		textureSlice.fill(0);

		auto UploadImage = [&](ClientAssetCookRegistry::TextureType textureType, Nz::UInt32 textureSlice, std::string_view filePath)
		{
			std::string assetPath = fmt::format("CookedAssets/{}", filePath);

			std::shared_ptr<Nz::Image> image = Nz::Image::LoadFromStream(*fs.GetFile(assetPath));
			if (!image)
				return false;

			for (Nz::UInt8 level = 0; level < image->GetLevelCount(); ++level)
			{
				m_blockTextures[textureType]->Update([&](void* pixelBuffer, Nz::UInt32 rowPitch, Nz::UInt32 depthPitch)
				{
					std::memcpy(pixelBuffer, image->GetConstPixels(level), Nz::PixelFormatInfo::ComputeSize(image->GetFormat(), image->GetWidth(level), image->GetHeight(level), 1));
					return true;
				}, Nz::Boxui(0, 0, textureSlice, image->GetWidth(level), image->GetHeight(level), 1), level);
			}

			return true;
		};

		std::vector<std::uint32_t> previewTextures(m_blocks.size());

		Nz::UInt32 blockIndex = 0;
		while (!blockTextures.empty())
		{
			const BlockTexture& blockTexture = blockTextures.front();

			for (const auto& [textureType, textureData] : blockTexture.textureData.iter_kv())
			{
				if (textureData->type != ClientAssetCookRegistry::TextureType::None &&
				    UploadImage(textureData->type, textureSlice[textureData->type], textureData->path))
				{
					Nz::Vector2i32& blockTextureSlice = Nz::AccessByOffset<Nz::Vector2i32&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + textureSliceOffsets[textureType]);
					blockTextureSlice = { static_cast<Nz::Int32>(textureData->type), static_cast<Nz::Int32>(textureSlice[textureData->type]) };

					textureSlice[textureData->type]++;
				}
			}

			blockTextures.erase(blockTextures.begin()); //< Destroy entry to free streams
			blockIndex++;
		}

		/*m_baseColorTexture = Nz::TextureAsset::CreateFromTexture(m_texture);

		m_previewTextures.resize(m_blocks.size());
		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			const auto& blockData = m_blocks[blockIndex];

			Nz::TextureViewInfo slotTexView = {
				.viewType = Nz::ImageType::E2D,
				.baseArrayLayer = previewTextures[blockIndex]
			};

			m_previewTextures[blockIndex] = Nz::TextureAsset::CreateView(m_baseColorTexture, slotTexView);
		}*/
	}
}
