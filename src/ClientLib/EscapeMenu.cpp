// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
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

		m_closeMenuButton = m_layout->Add<Nz::ButtonWidget>();
		m_closeMenuButton->UpdateText(Nz::SimpleTextDrawer::Draw("Close", 30));
		m_closeMenuButton->SetMaximumSize(m_closeMenuButton->GetPreferredSize());
		m_closeMenuButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			Hide();
		});

		m_disconnectButton = m_layout->Add<Nz::ButtonWidget>();
		m_disconnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Disconnect", 30));
		m_disconnectButton->SetMaximumSize(m_disconnectButton->GetPreferredSize());
		m_disconnectButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnDisconnect(this);
		});

		m_quitAppButton = m_layout->Add<Nz::ButtonWidget>();
		m_quitAppButton->UpdateText(Nz::SimpleTextDrawer::Draw("Exit application", 30));
		m_quitAppButton->SetMaximumSize(m_quitAppButton->GetPreferredSize());
		m_quitAppButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnQuitApp(this);
		});

		constexpr float padding = 20.f;
		constexpr float buttonPadding = 10.f;

		std::array buttons = { m_closeMenuButton, m_disconnectButton, m_quitAppButton };

		float maxWidth = 0.f;
		float height = 0.f;
		for (Nz::ButtonWidget* button : buttons)
		{
			maxWidth = std::max(maxWidth, button->GetPreferredWidth());
			height += button->GetPreferredHeight();
		}

		height += buttonPadding * (buttons.size() - 1);

		SetMinimumSize({ maxWidth, height });

		maxWidth += padding * 2.f;
		height += padding * 2.f;

		SetPreferredSize({ maxWidth, height });
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
