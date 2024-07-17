// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientBlockLibrary.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Image.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>

namespace tsom
{
	void ClientBlockLibrary::BuildTexture()
	{
		auto& fs = m_applicationBase.GetComponent<Nz::FilesystemAppComponent>();

		std::size_t sliceCount = m_textureIndices.size() + 1;

		constexpr std::size_t texSize = 128; // TODO: use texture size?

		Nz::Image baseColorArray(Nz::ImageType::E2D_Array, Nz::PixelFormat::RGBA8, texSize, texSize, sliceCount);
		baseColorArray.Fill(Nz::Color::White());

		Nz::Image normalArray(Nz::ImageType::E2D_Array, Nz::PixelFormat::RGBA8, texSize, texSize, sliceCount);
		normalArray.Fill(Nz::Color(0.5f, 0.5f, 1.0f));

		Nz::Image detailArray(Nz::ImageType::E2D_Array, Nz::PixelFormat::RGBA8, texSize, texSize, sliceCount);
		detailArray.Fill(Nz::Color(1.f, 0.f, 0.f, 1.f));

		for (auto&& [texPath, texIndex] : m_textureIndices)
		{
			Nz::ImageParams loadParams;
			loadParams.loadFormat = Nz::PixelFormat::RGBA8;

			std::shared_ptr<Nz::Image> baseColorImage = fs.Load<Nz::Image>("assets/" + texPath + ".png", loadParams);
			if (baseColorImage)
				baseColorArray.Copy(*baseColorImage, Nz::Boxui(baseColorImage->GetSize()), Nz::Vector3ui(0, 0, texIndex));

			std::shared_ptr<Nz::Image> normalImage = fs.Load<Nz::Image>("assets/" + texPath + "_n.png", loadParams);
			if (normalImage)
				normalArray.Copy(*normalImage, Nz::Boxui(normalImage->GetSize()), Nz::Vector3ui(0, 0, texIndex));

			std::shared_ptr<Nz::Image> detailImage = fs.Load<Nz::Image>("assets/" + texPath + "_s.png", loadParams);
			if (detailImage)
				detailArray.Copy(*detailImage, Nz::Boxui(detailImage->GetSize()), Nz::Vector3ui(0, 0, texIndex));
		}

		m_baseColorTexture = Nz::TextureAsset::CreateFromImage(std::move(baseColorArray), { .sRGB = true });
		m_normalTexture = Nz::TextureAsset::CreateFromImage(std::move(normalArray));
		m_detailTexture = Nz::TextureAsset::CreateFromImage(std::move(detailArray));

		m_previewTextures.resize(m_blocks.size());
		for (std::size_t blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex)
		{
			const auto& blockData = m_blocks[blockIndex];

			Nz::TextureViewInfo slotTexView = {
				.viewType = Nz::ImageType::E2D,
				.baseArrayLayer = blockData.texIndices[Direction::Up]
			};

			m_previewTextures[blockIndex] = Nz::TextureAsset::CreateView(m_baseColorTexture, slotTexView);
		}
	}
}
