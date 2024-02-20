// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/EscapeMenu.hpp>
#include <Nazara/TextRenderer/SimpleTextDrawer.hpp>

namespace tsom
{
	EscapeMenu::EscapeMenu(Nz::Canvas* canvas)
	{
		m_backgroundWidget = canvas->Add<Nz::BaseWidget>();
		m_backgroundWidget->EnableBackground(true);
		m_backgroundWidget->SetBackgroundColor(Nz::Color(0, 0, 0, 0.6f));

		m_closeMenuButton = m_backgroundWidget->Add<Nz::ButtonWidget>();
		m_closeMenuButton->UpdateText(Nz::SimpleTextDrawer::Draw("Close", 30));
		m_closeMenuButton->Resize(m_closeMenuButton->GetPreferredSize());
		m_closeMenuButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			Hide();
		});

		m_disconnectButton = m_backgroundWidget->Add<Nz::ButtonWidget>();
		m_disconnectButton->UpdateText(Nz::SimpleTextDrawer::Draw("Disconnect", 30));
		m_disconnectButton->Resize(m_disconnectButton->GetPreferredSize());
		m_disconnectButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnDisconnect(this);
		});

		m_quitAppButton = m_backgroundWidget->Add<Nz::ButtonWidget>();
		m_quitAppButton->UpdateText(Nz::SimpleTextDrawer::Draw("Exit application", 30));
		m_quitAppButton->Resize(m_quitAppButton->GetPreferredSize());
		m_quitAppButton->OnButtonTrigger.Connect([this](const Nz::ButtonWidget*)
		{
			OnQuitApp(this);
		});

		// Connect slots
		m_onCanvasResizedSlot.Connect(canvas->OnWidgetResized, [this](const Nz::BaseWidget*, const Nz::Vector2f&)
		{
			Layout();
		});
		Layout();

		Hide();
	}

	EscapeMenu::~EscapeMenu()
	{
		m_backgroundWidget->Destroy();
	}

	void EscapeMenu::Show(bool shouldOpen)
	{
		if (IsVisible() != shouldOpen)
		{
			m_backgroundWidget->Show(shouldOpen);

			Layout();
		}
	}

	void EscapeMenu::OnBackButtonPressed()
	{
		m_closeMenuButton->Show();
		m_disconnectButton->Show();
		m_quitAppButton->Show();

		Layout();
	}

	void EscapeMenu::Layout()
	{
		constexpr float padding = 20.f;
		constexpr float buttonPadding = 10.f;

		std::array buttons = { m_closeMenuButton, m_disconnectButton, m_quitAppButton };

		float maxWidth = 0.f;
		float height = 0.f;
		for (Nz::ButtonWidget* button : buttons)
		{
			maxWidth = std::max(maxWidth, button->GetWidth());
			height += button->GetHeight();
		}
		maxWidth += padding * 2.f;
		height += padding * 2.f + buttonPadding * (buttons.size() - 1);

		m_backgroundWidget->Resize({ maxWidth, height });

		float cursor = height - padding;
		for (Nz::ButtonWidget* button : buttons)
		{
			cursor -= button->GetHeight();

			button->SetPosition(0.f, cursor);
			button->CenterHorizontal();

			cursor -= buttonPadding;
		}

		m_backgroundWidget->Center();
	}
}
