// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Chatbox.hpp>
#include <CommonLib/GameConstants.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/TextRenderer/Font.hpp>
#include <Nazara/Widgets/RichTextAreaWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <spdlog/spdlog.h>

namespace tsom
{
	Chatbox::Chatbox(Nz::BaseWidget* parent) :
	BaseWidget(parent),
	m_chatEnteringBox(nullptr)
	{
		//TODO
		//Nz::FontRef chatboxFont = Nz::FontLibrary::Get("BW_Chatbox");
		//assert(chatboxFont);

		m_chatboxBackground = Add<Nz::BaseWidget>();
		m_chatboxBackground->SetBackgroundColor(Nz::Color(0.f, 0.f, 0.f, 0.5f));

		m_chatboxHistory = Add<Nz::RichTextAreaWidget>();
		m_chatboxHistory->EnableLineWrap(true);
		m_chatboxHistory->SetCharacterSize(22);
		m_chatboxHistory->SetTextColor(Nz::Color::White());
		m_chatboxHistory->SetTextOutlineColor(Nz::Color::Black());
		m_chatboxHistory->SetTextOutlineThickness(1.f);
		m_chatboxHistory->SetReadOnly(true);

		m_chatboxScrollArea = m_chatboxBackground->Add<Nz::ScrollAreaWidget>(m_chatboxHistory);
		m_chatboxScrollArea->Resize({ 480.f, 0.f });
		m_chatboxScrollArea->EnableScrollbar(false);

		m_chatEnteringBox = Add<Nz::TextAreaWidget>();
		m_chatEnteringBox->EnableBackground(true);
		m_chatEnteringBox->SetBackgroundColor(Nz::Color(1.f, 1.f, 1.f, 0.9f));
		m_chatEnteringBox->SetTextColor(Nz::Color::Black());
		//m_chatEnteringBox->SetTextFont(chatboxFont);
		m_chatEnteringBox->Hide();
		m_chatEnteringBox->SetMaximumTextLength(Constants::ChatMaxPlayerMessageLength);

		Refresh();
	}

	void Chatbox::Clear()
	{
		m_chatEntries.clear();
		m_chatboxHistory->Clear();
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
				m_chatboxBackground->EnableBackground(true);
				m_chatboxScrollArea->EnableScrollbar(true);
				m_chatEnteringBox->Show();
				m_chatEnteringBox->SetFocus();
			}
			else
			{
				m_chatboxBackground->EnableBackground(false);
				m_chatboxScrollArea->EnableScrollbar(false);
				m_chatEnteringBox->Clear();
				m_chatEnteringBox->Hide();
			}

			Refresh();
		}
	}

	void Chatbox::PrintMessage(std::vector<Item> items)
	{
		return PrintMessage(items, Constants::ChatPlayerMessageDisplayTime);
	}

	void Chatbox::PrintMessage(std::vector<Item> items, Nz::Time disappearTime)
	{
		if (m_chatEntries.size() >= Constants::ChatMaxLines)
			m_chatEntries.erase(m_chatEntries.begin());

		auto& chatEntry = m_chatEntries.emplace_back();
		chatEntry.disappearTime = Nz::Timestamp::Now() + disappearTime;
		chatEntry.items = std::move(items);

		Refresh();
	}

	void Chatbox::SendMessage()
	{
		std::string text = m_chatEnteringBox->GetText();
		if (!text.empty())
			OnChatMessage(text);
	}

	void Chatbox::SetFocus()
	{
		m_chatEnteringBox->SetFocus();
	}

	void Chatbox::Update()
	{
		Nz::Timestamp now = Nz::Timestamp::Now();
		if (now > m_nextDisappearTime)
			Refresh();
	}

	void Chatbox::Layout()
	{
		BaseWidget::Layout();

		Nz::Vector2f size = GetSize();

		m_chatEnteringBox->Resize({ size.x, m_chatEnteringBox->GetPreferredHeight() });
		m_chatEnteringBox->SetPosition({ 0.f, 0.f, 0.f });
		m_chatboxBackground->Resize({ size.x / 3.f, size.y / 3.f });
		m_chatboxBackground->SetPosition({ 5.f, m_chatEnteringBox->GetPosition().y + m_chatEnteringBox->GetHeight() + 5.f, 0.f });
	}

	void Chatbox::Refresh()
	{
		Nz::Timestamp now = Nz::Timestamp::Now();

		m_nextDisappearTime = Nz::Timestamp::FromNanoseconds(Nz::MaxValue<Nz::Int64>()); //< max timestamp

		m_chatboxHistory->Clear();
		for (const auto& entry : m_chatEntries)
		{
			if (now > entry.disappearTime)
			{
				if (!IsOpen())
					continue;
			}
			else
				m_nextDisappearTime = std::min(m_nextDisappearTime, entry.disappearTime);

			for (const Item& lineItem : entry.items)
			{
				std::visit([&](auto&& item)
				{
					using T = std::decay_t<decltype(item)>;

					if constexpr (std::is_same_v<T, ColorItem>)
					{
						m_chatboxHistory->SetTextColor(item.color);
					}
					else if constexpr (std::is_same_v<T, TextItem>)
					{
						if (!item.text.empty())
							m_chatboxHistory->AppendText(item.text);
					}
					else
						static_assert(Nz::AlwaysFalse<T>(), "non-exhaustive visitor");

				}, lineItem);
			}

			m_chatboxHistory->SetTextColor(Nz::Color::White());
			m_chatboxHistory->AppendText("\n");
		}

		m_chatboxHistory->Resize({ m_chatboxHistory->GetWidth(), m_chatboxHistory->GetPreferredHeight() });
		m_chatboxScrollArea->Resize({ m_chatboxBackground->GetWidth(), std::min(m_chatboxBackground->GetHeight(), m_chatboxHistory->GetHeight()) });

		m_chatboxScrollArea->ScrollToRatio(1.f);
	}
}
