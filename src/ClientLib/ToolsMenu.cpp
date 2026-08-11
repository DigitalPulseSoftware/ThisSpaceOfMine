// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ToolsMenu.hpp>

#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets.hpp>

#include <fmt/format.h>

namespace tsom
{
	ToolsMenu::ToolsMenu(Nz::BaseWidget* parent, Nz::FilesystemAppComponent& filesystem, uint8_t selectedTool) :
	BaseWidget(parent),
	m_hoveredTool(selectedTool),
	m_filesystem(filesystem)
	{
		//EnableBackground(true);
		//SetBackgroundColor(Nz::Color(0, 0, 0, 0.6f));

		for (size_t i = 0; i < m_imageMaterials.size(); i++) {
			std::shared_ptr<Nz::MaterialInstance>& imageMaterial = m_imageMaterials[i];
			imageMaterial = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic, Nz::MaterialInstancePreset::UI);
			imageMaterial->SetTextureProperty("BaseColorMap", m_filesystem.Open<Nz::TextureAsset>(fmt::format("CookedAssets/UI/ToolsMenu/ToolsMenu_Tool{0}.dds", i), { .sRGB = true }));
		}
		
		m_image = Add<Nz::ImageWidget>(m_imageMaterials[m_hoveredTool]);
		m_image->Resize({ 600.f, 600.f });

		for (size_t i = 0; i < m_toolLabelWidgets.size(); i++) {
			Nz::LabelWidget*& toolLabelWidget = m_toolLabelWidgets[i];
			toolLabelWidget = Add<Nz::LabelWidget>();
			toolLabelWidget->UpdateText(Nz::SimpleTextDrawer::Draw(m_toolTexts[i], 30, Nz::TextStyle_Regular, (i == m_hoveredTool ? Nz::Color::Black() : Nz::Color::White())));
		}

		Resize(m_image->GetSize());
	}

	void ToolsMenu::HandleMouseMoved(float mouseX, float mouseY, uint8_t selectedTool)
	{
		Nz::Vector3f centerPosition = m_image->GetGlobalPosition() + m_image->GetSize() / 2.f;
		Nz::Vector2f centeredMousePosition = { mouseX - centerPosition.x , mouseY - centerPosition.y };

		uint8_t previousHoveredTool = m_hoveredTool;
		if (centeredMousePosition.GetLength() >= m_image->GetSize()[0] / 4.f)
		{
			float angle = std::atan2(-centeredMousePosition.x, centeredMousePosition.y);
			m_hoveredTool = static_cast<uint8_t>(round(m_imageMaterials.size() * (angle / Nz::Pi<float>() + 1.f) / 2.f)) % m_imageMaterials.size();
		}
		else
		{
			m_hoveredTool = selectedTool;
		}

		HoverTool(m_hoveredTool, previousHoveredTool);
		Layout();
	}

	void ToolsMenu::HoverTool(uint8_t newToolIndex, uint8_t oldToolIndex)
	{
		m_image->SetMaterial(m_imageMaterials[newToolIndex % m_imageMaterials.size()]);
		m_toolLabelWidgets[oldToolIndex]->UpdateText(Nz::SimpleTextDrawer::Draw(m_toolTexts[oldToolIndex], 30, Nz::TextStyle_Regular, Nz::Color::White()));
		m_toolLabelWidgets[newToolIndex]->UpdateText(Nz::SimpleTextDrawer::Draw(m_toolTexts[newToolIndex], 30, Nz::TextStyle_Regular, Nz::Color::Black()));
	}

	void ToolsMenu::Layout()
	{
		BaseWidget::Layout();

		m_image->Center();

		float distFromCenter = m_image->GetSize()[0] * 3.f / 8.f;
		for (size_t i = 0; i < m_toolLabelWidgets.size(); i++) {
			Nz::LabelWidget*& toolLabelWidget = m_toolLabelWidgets[i];

			float angle = i * 2.f * Nz::Pi<float>() / m_toolLabelWidgets.size();

			toolLabelWidget->Center();
			toolLabelWidget->Move(Nz::Vector2f({ distFromCenter * sinf(angle), distFromCenter * cosf(angle) }));
		}
	}
}
