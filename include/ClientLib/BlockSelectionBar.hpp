// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_BLOCKSELECTIONBAR_HPP
#define TSOM_CLIENTLIB_BLOCKSELECTIONBAR_HPP

#include <ClientLib/Export.hpp>
#include <CommonLib/BlockIndex.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/Widgets/BaseWidget.hpp>
#include <variant>

namespace Nz
{
	class ImageWidget;
}

namespace tsom
{
	class ClientBlockLibrary;

	class TSOM_CLIENTLIB_API BlockSelectionBar : public Nz::BaseWidget
	{
		public:
			struct ColorItem
			{
				Nz::Color color;
			};

			struct TextItem
			{
				std::string text;
			};

			using Item = std::variant<ColorItem, TextItem>;

			BlockSelectionBar(Nz::BaseWidget* parent, const ClientBlockLibrary& blockLibrary);
			BlockSelectionBar(const BlockSelectionBar&) = delete;
			BlockSelectionBar(BlockSelectionBar&&) = delete;
			~BlockSelectionBar() = default;

			inline BlockIndex GetSelectedBlock() const;

			void SelectNext();
			void SelectPrevious();

			BlockSelectionBar& operator=(const BlockSelectionBar&) = delete;
			BlockSelectionBar& operator=(BlockSelectionBar&&) = delete;

			static constexpr float InventoryTileSize = 96.f;
			static constexpr float Padding = 5.f;

		private:
			void Layout();

			std::size_t m_selectedIndex;
			std::vector<Nz::ImageWidget*> m_inventorySprites;
			BlockIndex m_selectedBlockIndex;
			const ClientBlockLibrary& m_blockLibrary;
	};
}

#include <ClientLib/BlockSelectionBar.inl>

#endif // TSOM_CLIENTLIB_BLOCKSELECTIONBAR_HPP
