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
			AmbientOcclusion,
			BaseColor,
			Height,
			Metalness,
			Normal,
			Roughness,

			Max = Roughness
		};

		struct GlobalBlockBufferEntryOffsets
		{
			nzsl::FieldOffsets fieldOffsets;
			std::size_t ambientOcclusionMapIndex;
			std::size_t baseColorMapIndex;
			std::size_t heightMapIndex;
			std::size_t metalness;
			std::size_t metalnessMapIndex;
			std::size_t normalMapIndex;
			std::size_t roughness;
			std::size_t roughnessMapIndex;
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

			bufferOffsets.ambientOcclusionMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.baseColorMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.heightMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.metalness = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.metalnessMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.normalMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.roughness = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);
			bufferOffsets.roughnessMapIndex = bufferOffsets.fieldOffsets.AddField(nzsl::StructFieldType::Float1);

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

		struct BlockTexture
		{
			Nz::EnumArray<TextureType, std::shared_ptr<Nz::Stream>> streams;
		};

		Nz::UInt8* blockBufferPtr = static_cast<Nz::UInt8*>(m_globalBlockBufferPtr) + s_blockBufferOffsets.entries;

		Nz::UInt32 sliceCount = 0;
		std::vector<BlockTexture> blockTextures;
		blockTextures.reserve(m_blocks.size());

		for (const BlockData& blockData : m_blocks)
		{
			std::size_t blockIndex = blockTextures.size();
			auto& blockTexture = blockTextures.emplace_back();
			blockTexture.streams[TextureType::BaseColor] = fs.GetFile(fmt::format("assets/{}.png", blockData.basePath));
			blockTexture.streams[TextureType::AmbientOcclusion] = fs.GetFile(fmt::format("assets/{}_ao.png", blockData.basePath));
			blockTexture.streams[TextureType::Height] = fs.GetFile(fmt::format("assets/{}_height.png", blockData.basePath));
			blockTexture.streams[TextureType::Metalness] = fs.GetFile(fmt::format("assets/{}_metallic.png", blockData.basePath));
			blockTexture.streams[TextureType::Normal] = fs.GetFile(fmt::format("assets/{}_normal.png", blockData.basePath));
			blockTexture.streams[TextureType::Roughness] = fs.GetFile(fmt::format("assets/{}_roughness.png", blockData.basePath));

			Nz::AccessByOffset<float&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.metalness) = blockData.metalness;
			Nz::AccessByOffset<float&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + s_blockBufferEntryOffsets.roughness) = blockData.roughness;

			for (const auto& stream : blockTexture.streams)
			{
				if (stream)
					sliceCount++;
			}
		}

		constexpr std::size_t texSize = 2048; // TODO: use texture size?

		m_texture = renderDevice.InstantiateTexture({
			.pixelFormat = Nz::PixelFormat::RGBA8,
			.type = Nz::ImageType::E2D_Array,
			.layerCount = sliceCount,
			.height = texSize,
			.width = texSize
		});

		Nz::ImageParams loadParams;
		loadParams.loadFormat = Nz::PixelFormat::RGBA8;

		constexpr Nz::EnumArray<TextureType, std::size_t> textureSliceOffsets = {
			s_blockBufferEntryOffsets.ambientOcclusionMapIndex,
			s_blockBufferEntryOffsets.baseColorMapIndex,
			s_blockBufferEntryOffsets.heightMapIndex,
			s_blockBufferEntryOffsets.metalnessMapIndex,
			s_blockBufferEntryOffsets.normalMapIndex,
			s_blockBufferEntryOffsets.roughnessMapIndex
		};

		Nz::UInt32 textureSlice = 0;
		auto UploadImage = [&](Nz::UInt32 blockIndex, TextureType textureType, Nz::Stream& stream)
		{
			std::shared_ptr<Nz::Image> image = Nz::Image::LoadFromStream(stream, loadParams);
			if (!image)
				return false;

			m_texture->Update([&](void* pixelBuffer, Nz::UInt32 rowPitch, Nz::UInt32 depthPitch)
			{
				Nz::ImageUtils::Copy(pixelBuffer, image->GetConstPixels(), image->GetFormat(), image->GetWidth(), image->GetHeight(), 1, rowPitch, depthPitch, 0, 0);
				return true;
			}, Nz::Boxui(0, 0, textureSlice, image->GetWidth(), image->GetHeight(), 1), 0);

			return true;
		};

		std::vector<std::uint32_t> previewTextures(m_blocks.size());

		Nz::UInt32 blockIndex = 0;
		while (!blockTextures.empty())
		{
			const BlockTexture& blockTexture = blockTextures.front();

			for (const auto& [textureType, stream] : blockTexture.streams.iter_kv())
			{
				float& blockTextureSlice = Nz::AccessByOffset<float&>(blockBufferPtr, blockIndex * s_blockBufferEntryOffsets.fieldOffsets.GetAlignedSize() + textureSliceOffsets[textureType]);
				if (stream && UploadImage(blockIndex, textureType, *stream))
				{
					if (textureType == TextureType::BaseColor)
						previewTextures[blockIndex] = textureSlice;

					blockTextureSlice = textureSlice++;
				}
				else
					blockTextureSlice = -1.0f;
			}

			blockTextures.erase(blockTextures.begin()); //< Destroy entry to free streams
			blockIndex++;
		}

		m_texture->BuildMipmaps();

		m_baseColorTexture = Nz::TextureAsset::CreateFromTexture(m_texture);

		m_previewTextures.resize(m_blocks.size());
		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			const auto& blockData = m_blocks[blockIndex];

			Nz::TextureViewInfo slotTexView = {
				.viewType = Nz::ImageType::E2D,
				.baseArrayLayer = previewTextures[blockIndex]
			};

			m_previewTextures[blockIndex] = Nz::TextureAsset::CreateView(m_baseColorTexture, slotTexView);
		}
	}
}
