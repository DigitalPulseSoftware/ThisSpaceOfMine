// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/EscapeMenu.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>
#include <Nazara/Widgets/BoxLayout.hpp>

namespace tsom
{
	EscapeMenu::EscapeMenu(Nz::BaseWidget* parent) :
	BaseWidget(parent)
	{
		EnableBackground(true);
		SetBackgroundColor(Nz::Color(0, 0, 0, 0.6f));

		m_layout = Add<Nz::BoxLayout>(Nz::BoxLayoutOrientation::TopToBottom);

		AddButton("Close", [this]{ Hide(); });
	}

	void EscapeMenu::AddButton(std::string buttonName, std::function<void()> callback)
	{
		Nz::ButtonWidget* button = m_layout->Add<Nz::ButtonWidget>();
		button->UpdateText(Nz::SimpleTextDrawer::Draw(std::move(buttonName), 30, Nz::TextStyle_Regular, Nz::Color::Black()));
		button->SetMaximumSize(button->GetPreferredSize());
		button->OnButtonTrigger.Connect([this, cb = std::move(callback)](const Nz::ButtonWidget*)
		{
			cb();
		});

		m_buttons.push_back(button);

		constexpr float padding = 20.f;
		constexpr float buttonPadding = 10.f;

		float maxWidth = 0.f;
		float height = 0.f;
		for (Nz::ButtonWidget* button : m_buttons)
		{
			maxWidth = std::max(maxWidth, button->GetPreferredWidth());
			height += button->GetPreferredHeight();
		}

		height += buttonPadding * (m_buttons.size() - 1);

		SetMinimumSize({ maxWidth, height });

		maxWidth += padding * 2.f;
		height += padding * 2.f;

		SetPreferredSize({ maxWidth, height });

		Layout();
	}

	void EscapeMenu::Layout()
	{
		BaseWidget::Layout();

		constexpr float padding = 20.f;
		constexpr float buttonPadding = 10.f;

		m_layout->Resize(GetMinimumSize());
		m_layout->Center();
	}
}
