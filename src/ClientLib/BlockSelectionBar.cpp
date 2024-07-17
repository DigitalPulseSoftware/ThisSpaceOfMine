// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/BlockSelectionBar.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/TextRenderer/Font.hpp>
#include <Nazara/Widgets/ImageWidget.hpp>
#include <Nazara/Widgets/RichTextAreaWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <fmt/format.h>

namespace tsom
{
	constexpr std::array<std::string_view, 7> s_selectableBlocks = { "dirt", "grass", "stone", "snow", "stone_bricks", "planks", "debug" };

	BlockSelectionBar::BlockSelectionBar(Nz::BaseWidget* parent, const ClientBlockLibrary& blockLibrary) :
	BaseWidget(parent),
	m_selectedIndex(0),
	m_blockLibrary(blockLibrary)
	{
		const auto& blockColorMap = m_blockLibrary.GetBaseColorTexture();

		for (std::string_view blockName : s_selectableBlocks)
		{
			bool active = m_selectedIndex == m_inventorySprites.size();

			BlockIndex blockIndex = m_blockLibrary.GetBlockIndex(blockName);

			std::shared_ptr<Nz::MaterialInstance> slotMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
			slotMat->SetTextureProperty("BaseColorMap", m_blockLibrary.GetPreviewTexture(blockIndex));

			Nz::ImageWidget* imageWidget = Add<Nz::ImageWidget>(slotMat);
			imageWidget->SetColor((active) ? Nz::Color::White() : Nz::Color::sRGBToLinear(Nz::Color::Gray()));
			imageWidget->Resize({ InventoryTileSize, InventoryTileSize });

			m_inventorySprites.push_back(imageWidget);
		}

		m_selectedBlockIndex = m_blockLibrary.GetBlockIndex(s_selectableBlocks[m_selectedIndex]);
	}

	void BlockSelectionBar::SelectNext()
	{
		m_inventorySprites[m_selectedIndex]->SetColor(Nz::Color::sRGBToLinear(Nz::Color::Gray()));

		m_selectedIndex++;
		if (m_selectedIndex >= s_selectableBlocks.size())
			m_selectedIndex = 0;

		m_inventorySprites[m_selectedIndex]->SetColor(Nz::Color::White());
		m_selectedBlockIndex = m_blockLibrary.GetBlockIndex(s_selectableBlocks[m_selectedIndex]);
	}

	void BlockSelectionBar::SelectPrevious()
	{
		m_inventorySprites[m_selectedIndex]->SetColor(Nz::Color::sRGBToLinear(Nz::Color::Gray()));

		if (m_selectedIndex > 0)
			m_selectedIndex--;
		else
			m_selectedIndex = s_selectableBlocks.size() - 1;

		m_inventorySprites[m_selectedIndex]->SetColor(Nz::Color::White());
		m_selectedBlockIndex = m_blockLibrary.GetBlockIndex(s_selectableBlocks[m_selectedIndex]);
	}

	void BlockSelectionBar::Layout()
	{
		BaseWidget::Layout();

		float offset = GetWidth() / 2.f - (s_selectableBlocks.size() * (InventoryTileSize + Padding)) * 0.5f;
		for (Nz::ImageWidget* image : m_inventorySprites)
		{
			image->SetPosition({ offset, 0.f });
			offset += (InventoryTileSize + Padding);
		}
	}
}
