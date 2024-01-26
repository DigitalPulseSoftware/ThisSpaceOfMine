// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <ClientLib/Chatbox.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/Utility/Font.hpp>
#include <Nazara/Widgets/RichTextAreaWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <fmt/format.h>

namespace tsom
{
	Chatbox::Chatbox(Nz::RenderTarget& renderTarget, Nz::Canvas* canvas) :
	m_chatEnteringBox(nullptr)
	{
		//TODO
		//Nz::FontRef chatboxFont = Nz::FontLibrary::Get("BW_Chatbox");
		//assert(chatboxFont);

		m_chatBox = canvas->Add<Nz::RichTextAreaWidget>();
		m_chatBox->EnableBackground(false);
		m_chatBox->EnableLineWrap(true);
		m_chatBox->SetBackgroundColor(Nz::Color(0.f, 0.f, 0.f, 0.2f));
		m_chatBox->SetCharacterSize(22);
		m_chatBox->SetTextColor(Nz::Color::White());
		//m_chatBox->SetTextFont(chatboxFont);
		m_chatBox->SetTextOutlineColor(Nz::Color::Black());
		m_chatBox->SetTextOutlineThickness(1.f);
		m_chatBox->SetReadOnly(true);

		m_chatboxScrollArea = canvas->Add<Nz::ScrollAreaWidget>(m_chatBox);
		m_chatboxScrollArea->Resize({ 480.f, 0.f });
		m_chatboxScrollArea->EnableScrollbar(false);

		m_chatEnteringBox = canvas->Add<Nz::TextAreaWidget>();
		m_chatEnteringBox->EnableBackground(true);
		m_chatEnteringBox->SetBackgroundColor(Nz::Color(1.f, 1.f, 1.f, 0.6f));
		m_chatEnteringBox->SetTextColor(Nz::Color::Black());
		//m_chatEnteringBox->SetTextFont(chatboxFont);
		m_chatEnteringBox->Hide();
		m_chatEnteringBox->SetMaximumTextLength(Constants::ChatMaxMessageLength);

		// Connect every slot
		m_onTargetChangeSizeSlot.Connect(renderTarget.OnRenderTargetSizeChange, this, &Chatbox::OnRenderTargetSizeChange);

		OnRenderTargetSizeChange(&renderTarget, renderTarget.GetSize());
	}

	Chatbox::~Chatbox()
	{
		m_chatboxScrollArea->Destroy();

		if (m_chatEnteringBox)
			m_chatEnteringBox->Destroy();
	}

	void Chatbox::Clear()
	{
		m_chatLines.clear();
		m_chatBox->Clear();
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
		// Log message
		{
			std::string textMessage;

			for (const Item& messageItem : message)
			{
				std::visit([&](auto&& item)
				{
					using T = std::decay_t<decltype(item)>;

					if constexpr (std::is_same_v<T, TextItem>)
						textMessage += item.text;

				}, messageItem);
			}

			fmt::print("{0}\n", textMessage);
		}

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

	void Chatbox::OnRenderTargetSizeChange(const Nz::RenderTarget* renderTarget, const Nz::Vector2ui& newSize)
	{
		Nz::Vector2f size = Nz::Vector2f(newSize);

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
