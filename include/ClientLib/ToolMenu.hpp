// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_TOOLMENU_HPP
#define TSOM_CLIENTLIB_TOOLMENU_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Widgets/Canvas.hpp>

#include <string>

namespace Nz
{
	class ApplicationBase;
	class ImageWidget;
	class LabelWidget;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API ToolMenu : public Nz::BaseWidget
	{
		public:
			ToolMenu(Nz::BaseWidget* parent, Nz::ApplicationBase& app, std::size_t selectedTool);
			ToolMenu(const ToolMenu&) = delete;
			ToolMenu(ToolMenu&&) = delete;
			~ToolMenu() = default;

			inline std::size_t GetHoveredTool() const;

			void HandleMouseMoved(float mouseX, float mouseY, std::size_t selectedTool);

			ToolMenu& operator=(const ToolMenu&) = delete;
			ToolMenu& operator=(ToolMenu&&) = delete;

		private:
			void HoverTool(std::size_t toolIndex, std::size_t oldToolIndex);

			void Layout() override;

			struct ToolEntry
			{
				std::shared_ptr<Nz::MaterialInstance> material;
				Nz::LabelWidget* toolLabel;
			};

			std::array<ToolEntry, 6> m_toolEntries;
			std::size_t m_hoveredTool;
			Nz::ApplicationBase& m_app;
			Nz::ImageWidget* m_image;
	};
}

#include <ClientLib/ToolMenu.inl>

#endif // TSOM_CLIENTLIB_TOOLMENU_HPP
