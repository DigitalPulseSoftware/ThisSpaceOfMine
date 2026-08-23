// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/BlockSelectionBar.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/Widgets/ImageWidget.hpp>
#include <NazaraUtils/Assert.hpp>

namespace tsom
{
	constexpr std::array<std::string_view, 19> s_selectableBlocks = { "dirt", "grass", "stone", "snow", "hull", "stone_bricks", "planks", "copper_block", "glass", "bark", "cliff_rocks", "rock", "wood_floor", "white_bricks", "gold", "metal", "metal_plates", "brickswall", "floor_tiles"};

	BlockSelectionBar::BlockSelectionBar(Nz::BaseWidget* parent, const ClientBlockLibrary& blockLibrary) :
	BaseWidget(parent),
	m_selectedIndex(0),
	m_blockLibrary(blockLibrary)
	{
		for (std::string_view blockName : s_selectableBlocks)
		{
			bool active = m_selectedIndex == m_inventorySprites.size();

			BlockIndex blockIndex = m_blockLibrary.GetBlockIndex(blockName);
			NazaraAssertMsg(blockIndex != InvalidBlockIndex, "%s is not a valid block name", blockName.data());

			std::shared_ptr<Nz::MaterialInstance> slotMat = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic, Nz::MaterialInstancePreset::UI);
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

	void BlockSelectionBar::SelectPickedBlock(tsom::BlockIndex pickedBlockIndex)
	{
		if (!m_blockLibrary.IsValidBlock(pickedBlockIndex))
			return;

		auto it = std::find(s_selectableBlocks.begin(), s_selectableBlocks.end(), m_blockLibrary.GetBlockData(pickedBlockIndex).name);
		if (it == s_selectableBlocks.end())
			return;

		m_inventorySprites[m_selectedIndex]->SetColor(Nz::Color::sRGBToLinear(Nz::Color::Gray()));
		m_selectedIndex = std::distance(s_selectableBlocks.begin(), it);
		m_inventorySprites[m_selectedIndex]->SetColor(Nz::Color::White());
		m_selectedBlockIndex = pickedBlockIndex;
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
