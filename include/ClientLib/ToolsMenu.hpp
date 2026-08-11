// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLSMENU_HPP
#define TSOM_CLIENTLIB_TOOLSMENU_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Widgets/Canvas.hpp>

#include <string>

namespace Nz
{
	class FilesystemAppComponent;
	class ImageWidget;
	class LabelWidget;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ToolsMenu : public Nz::BaseWidget
	{
		public:
			ToolsMenu(Nz::BaseWidget* parent, Nz::FilesystemAppComponent& filesystem, uint8_t selectedTool);
			ToolsMenu(const ToolsMenu&) = delete;
			ToolsMenu(ToolsMenu&&) = delete;
			~ToolsMenu() = default;

			ToolsMenu& operator=(const ToolsMenu&) = delete;
			ToolsMenu& operator=(ToolsMenu&&) = delete;

			void HandleMouseMoved(float mouseX, float mouseY, uint8_t selectedTool);

			inline const uint8_t GetHoveredTool() const;

		private:
			void Layout() override;

			void HoverTool(uint8_t toolIndex, uint8_t oldToolIndex);

			uint8_t m_hoveredTool;

			Nz::FilesystemAppComponent& m_filesystem;

			std::array<std::shared_ptr<Nz::MaterialInstance>, 6> m_imageMaterials;
			Nz::ImageWidget* m_image;

			std::array<std::string, 6> m_toolTexts = { "None", "Block", "Place\nEntity", "Remove\nEntity", "Connect", "Grab" };
			std::array<Nz::LabelWidget*, 6> m_toolLabelWidgets;
	};
}

#include <ClientLib/ToolsMenu.inl>

#endif // TSOM_CLIENTLIB_TOOLSMENU_HPP
