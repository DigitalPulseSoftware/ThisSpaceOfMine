// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Chatbox.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/TextRenderer/Font.hpp>
#include <Nazara/Widgets/RichTextAreaWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <fmt/format.h>

namespace tsom
{
	Chatbox::Chatbox(Nz::BaseWidget* parent) :
	BaseWidget(parent),
	m_chatEnteringBox(nullptr)
	{
		//TODO
		//Nz::FontRef chatboxFont = Nz::FontLibrary::Get("BW_Chatbox");
		//assert(chatboxFont);

		m_chatBox = Add<Nz::RichTextAreaWidget>();
		m_chatBox->EnableBackground(false);
		m_chatBox->EnableLineWrap(true);
		m_chatBox->SetBackgroundColor(Nz::Color(0.f, 0.f, 0.f, 0.2f));
		m_chatBox->SetCharacterSize(22);
		m_chatBox->SetTextColor(Nz::Color::White());
		//m_chatBox->SetTextFont(chatboxFont);
		m_chatBox->SetTextOutlineColor(Nz::Color::Black());
		m_chatBox->SetTextOutlineThickness(1.f);
		m_chatBox->SetReadOnly(true);

		m_chatboxScrollArea = Add<Nz::ScrollAreaWidget>(m_chatBox);
		m_chatboxScrollArea->Resize({ 480.f, 0.f });
		m_chatboxScrollArea->EnableScrollbar(false);

		m_chatEnteringBox = Add<Nz::TextAreaWidget>();
		m_chatEnteringBox->EnableBackground(true);
		m_chatEnteringBox->SetBackgroundColor(Nz::Color(1.f, 1.f, 1.f, 0.6f));
		m_chatEnteringBox->SetTextColor(Nz::Color::Black());
		//m_chatEnteringBox->SetTextFont(chatboxFont);
		m_chatEnteringBox->Hide();
		m_chatEnteringBox->SetMaximumTextLength(Constants::ChatMaxPlayerMessageLength);
	}

	void Chatbox::Clear()
	{
		m_chatLines.clear();
		m_chatBox->Clear();
	}

	bool Chatbox::IsOpen() const
	{
		return m_chatEnteringBox->IsVisible();
	}

	bool Chatbox::IsTyping() const
	{
		return m_chatEnteringBox->IsVisible();
	}

	void Chatbox::Open(bool shouldOpen)
	{
		if (IsOpen() != shouldOpen)
		{
			if (shouldOpen)
			{
				m_chatBox->EnableBackground(true);
				m_chatboxScrollArea->EnableScrollbar(true);
				m_chatEnteringBox->Show(true);
				m_chatEnteringBox->SetFocus();
			}
			else
			{
				m_chatBox->EnableBackground(false);
				m_chatboxScrollArea->EnableScrollbar(false);
				m_chatEnteringBox->Clear();
				m_chatEnteringBox->Hide();
			}
		}
	}

	void Chatbox::PrintMessage(std::vector<Item> message)
	{
		m_chatLines.emplace_back(std::move(message));
		if (m_chatLines.size() > Constants::ChatMaxLines)
			m_chatLines.erase(m_chatLines.begin());

		Refresh();
	}

	void Chatbox::SendMessage()
	{
		std::string text = m_chatEnteringBox->GetText();
		if (!text.empty())
			OnChatMessage(text);
	}

	void Chatbox::Layout()
	{
		BaseWidget::Layout();

		Nz::Vector2f size = GetSize();

		m_chatEnteringBox->Resize({ size.x, 40.f });
		m_chatEnteringBox->SetPosition({ 0.f, 0.f, 0.f });
		m_chatboxScrollArea->Resize({ size.x / 3.f, size.y / 3.f });
		m_chatboxScrollArea->SetPosition({ 5.f, m_chatEnteringBox->GetPosition().y + m_chatEnteringBox->GetHeight() + 5.f, 0.f });
	}

	void Chatbox::Refresh()
	{
		m_chatBox->Clear();
		for (const auto& lineItems : m_chatLines)
		{
			for (const Item& lineItem : lineItems)
			{
				std::visit([&](auto&& item)
				{
					using T = std::decay_t<decltype(item)>;

					if constexpr (std::is_same_v<T, ColorItem>)
					{
						m_chatBox->SetTextColor(item.color);
					}
					else if constexpr (std::is_same_v<T, TextItem>)
					{
						if (!item.text.empty())
							m_chatBox->AppendText(item.text);
					}
					else
						static_assert(Nz::AlwaysFalse<T>(), "non-exhaustive visitor");

				}, lineItem);
			}

			m_chatBox->SetTextColor(Nz::Color::White());
			m_chatBox->AppendText("\n");
		}

		m_chatBox->Resize({ m_chatBox->GetWidth(), m_chatBox->GetPreferredHeight() });
		m_chatboxScrollArea->Resize(m_chatboxScrollArea->GetSize()); // force layout update
		m_chatboxScrollArea->SetPosition({ 5.f, m_chatEnteringBox->GetPosition().y + m_chatEnteringBox->GetHeight() + 5.f, 0.f});

		m_chatboxScrollArea->ScrollToRatio(1.f);
	}
}
