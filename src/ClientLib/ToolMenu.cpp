// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ToolMenu.hpp>
#include <Nazara/Widgets.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <fmt/format.h>

namespace tsom
{
	namespace
	{
		constexpr std::array<std::string_view, 6> s_toolTexts = { "None", "Block", "Place\nEntity", "Remove\nEntity", "Connect", "Grab" };
	}

	ToolMenu::ToolMenu(Nz::BaseWidget* parent, Nz::ApplicationBase& app, std::size_t selectedTool) :
	BaseWidget(parent),
	m_hoveredTool(selectedTool),
	m_app(app)
	{
		auto& filesystem = m_app.GetComponent<Nz::FilesystemAppComponent>();

		for (size_t i = 0; i < m_toolEntries.size(); i++)
		{
			ToolEntry& entry = m_toolEntries[i];
			entry.material = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic, Nz::MaterialInstancePreset::UI);
			entry.material->SetTextureProperty("BaseColorMap", filesystem.Open<Nz::TextureAsset>(fmt::format("CookedAssets/UI/ToolsMenu/ToolsMenu_Tool{0}.dds", i), { .sRGB = true }));

			entry.toolLabel = Add<Nz::LabelWidget>();
			entry.toolLabel->UpdateText(Nz::SimpleTextDrawer::Draw(std::string(s_toolTexts[i]), 30, Nz::TextStyle_Regular, (i == m_hoveredTool ? Nz::Color::Black() : Nz::Color::White())));
		}
	
		m_image = Add<Nz::ImageWidget>(m_toolEntries[m_hoveredTool].material);
		m_image->Resize({ 600.f, 600.f });

		Resize(m_image->GetSize());
	}

	void ToolMenu::HandleMouseMoved(float mouseX, float mouseY, std::size_t selectedTool)
	{
		Nz::Vector3f centerPosition = m_image->GetGlobalPosition() + m_image->GetSize() / 2.f;
		Nz::Vector2f centeredMousePosition = { mouseX - centerPosition.x , mouseY - centerPosition.y };

		std::size_t previousHoveredTool = m_hoveredTool;
		if (centeredMousePosition.GetLength() >= m_image->GetSize()[0] / 4.f)
		{
			float angle = std::atan2(-centeredMousePosition.x, centeredMousePosition.y);
			m_hoveredTool = static_cast<std::size_t>(std::round(m_toolEntries.size() * (angle / Nz::Pi<float>() + 1.f) / 2.f)) % m_toolEntries.size();
		}
		else
		{
			m_hoveredTool = selectedTool;
		}

		HoverTool(m_hoveredTool, previousHoveredTool);
		Layout();
	}

	void ToolMenu::HoverTool(std::size_t newToolIndex, std::size_t oldToolIndex)
	{
		m_image->SetMaterial(m_toolEntries[newToolIndex % m_toolEntries.size()].material);
		m_toolEntries[oldToolIndex].toolLabel->UpdateText(Nz::SimpleTextDrawer::Draw(std::string(s_toolTexts[oldToolIndex]), 30, Nz::TextStyle_Regular, Nz::Color::White()));
		m_toolEntries[newToolIndex].toolLabel->UpdateText(Nz::SimpleTextDrawer::Draw(std::string(s_toolTexts[newToolIndex]), 30, Nz::TextStyle_Regular, Nz::Color::Black()));
	}

	void ToolMenu::Layout()
	{
		BaseWidget::Layout();

		m_image->Center();

		float distFromCenter = m_image->GetSize()[0] * 3.f / 8.f;
		for (std::size_t i = 0; i < m_toolEntries.size(); i++)
		{
			Nz::TurnAnglef angle(float(i) / m_toolEntries.size());

			auto [s, c] = angle.GetSinCos();

			m_toolEntries[i].toolLabel->Center();
			m_toolEntries[i].toolLabel->Move(Nz::Vector2f({ distFromCenter * s, distFromCenter * c }));
		}
	}
}
