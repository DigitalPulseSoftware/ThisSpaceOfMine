// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Console.hpp>
#include <Nazara/Widgets/RichTextAreaWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <fmt/format.h>

namespace tsom
{
	Console::Console(Nz::BaseWidget* parent) :
	BaseWidget(parent),
	m_historyPosition(0),
	m_maxHistoryLines(200)
	{
		m_contentText = Add<Nz::RichTextAreaWidget>();
		m_contentText->EnableBackground(false);
		m_contentText->EnableLineWrap(true);
		m_contentText->SetBackgroundColor(Nz::Color(0.f, 0.f, 0.f, 0.5f));
		m_contentText->SetCharacterSize(12);
		m_contentText->SetTextColor(Nz::Color::White());
		m_contentText->SetTextOutlineColor(Nz::Color::Black());
		m_contentText->SetTextOutlineThickness(1.f);
		m_contentText->SetReadOnly(true);

		m_contentScrollArea = Add<Nz::ScrollAreaWidget>(m_contentText);
		m_contentScrollArea->Resize({ 480.f, 0.f });
		m_contentScrollArea->EnableScrollbar(false);

		m_inputArea = Add<Nz::TextAreaWidget>();
		m_inputArea->EnableBackground(true);
		m_inputArea->SetBackgroundColor(Nz::Color(1.f, 1.f, 1.f, 0.9f));
		m_inputArea->SetTextColor(Nz::Color::Black());
		m_inputArea->SetCharacterSize(14);
		m_inputArea->SetText(std::string(CommandPrefix));

		m_inputArea->OnTextAreaKeyReturn.Connect([this](const Nz::AbstractTextAreaWidget* textArea, bool* ignoreDefaultAction)
		{
			NazaraAssert(textArea == m_inputArea, "Unexpected signal from an other text area");

			*ignoreDefaultAction = true;

			std::string input = m_inputArea->GetText();
			std::string_view inputCmd = input.substr(CommandPrefix.size());
			m_inputArea->SetText(std::string(CommandPrefix));

			m_historyPosition = m_commandHistory.size();
			PrintMessage(std::move(input)); //< With the input prefix

			OnCommand(inputCmd);
		});

		// Protect input prefix from erasure/selection
		m_inputArea->SetCursorPosition(CommandPrefix.size());

		m_inputArea->OnTextAreaCursorMove.Connect([](const Nz::AbstractTextAreaWidget* textArea, Nz::Vector2ui* newCursorPos)
		{
			if (newCursorPos->y == 0)
				newCursorPos->x = std::max(newCursorPos->x, static_cast<unsigned int>(CommandPrefix.size()));
		});

		m_inputArea->OnTextAreaSelection.Connect([](const Nz::AbstractTextAreaWidget* textArea, Nz::Vector2ui* start, Nz::Vector2ui* end)
		{
			if (start->y == 0)
				start->x = std::max(start->x, static_cast<unsigned int>(CommandPrefix.size()));

			if (end->y == 0)
				end->x = std::max(end->x, static_cast<unsigned int>(CommandPrefix.size()));
		});

		m_inputArea->OnTextAreaKeyBackspace.Connect([](const Nz::AbstractTextAreaWidget* textArea, bool* ignoreDefaultAction)
		{
			if (textArea->GetGlyphIndex() < CommandPrefix.size())
				*ignoreDefaultAction = true;
		});

		// Handle history
		m_inputArea->OnTextAreaKeyUp.Connect([this] (const Nz::AbstractTextAreaWidget* textArea, bool* ignoreDefaultAction)
		{
			*ignoreDefaultAction = true;

			if (m_commandHistory.empty())
				return;

			if (m_historyPosition > 0)
				m_historyPosition--;

			m_inputArea->SetText(fmt::format("{}{}", CommandPrefix, m_commandHistory[m_historyPosition]));
		});

		m_inputArea->OnTextAreaKeyDown.Connect([this] (const Nz::AbstractTextAreaWidget* textArea, bool* ignoreDefaultAction)
		{
			*ignoreDefaultAction = true;

			if (m_commandHistory.empty())
				return;

			if (++m_historyPosition >= m_commandHistory.size())
				m_historyPosition = 0;

			m_inputArea->SetText(fmt::format("{}{}", CommandPrefix, m_commandHistory[m_historyPosition]));
		});
	}

	void Console::Clear()
	{
		m_contentLines.clear();
		m_contentText->Clear();
	}

	const Nz::Color& Console::GetBackgroundColor() const
	{
		return m_contentText->GetBackgroundColor();
	}

	unsigned int Console::GetCharacterSize() const
	{
		return m_inputArea->GetCharacterSize();
	}

	void Console::PrintMessage(std::string message, const Nz::Color& color)
	{
		m_contentLines.push_back({ .color = color, .text = std::move(message) });
		if (m_contentLines.size() > m_maxHistoryLines)
			m_contentLines.erase(m_contentLines.begin());

		m_contentText->SetTextColor(color);
		m_contentText->AppendText(m_contentLines.back().text);
		m_contentText->AppendText("\n");

		m_contentScrollArea->ScrollToRatio(1.f);
	}

	void Console::SetBackgroundColor(const Nz::Color& backgroundColor)
	{
		m_contentText->SetBackgroundColor(backgroundColor);
	}

	void Console::SetCharacterSize(unsigned int size)
	{
		m_inputArea->SetCharacterSize(size);

		RefreshContent();
	}

	void Console::Layout()
	{
		BaseWidget::Layout();

		Nz::Vector2f size = GetSize();

		m_inputArea->Resize({ size.x, m_inputArea->GetPreferredHeight() });
		m_inputArea->SetPosition({ 0.f, 0.f, 0.f });
		m_contentScrollArea->Resize({ size.x, size.y - m_inputArea->GetHeight()});
		m_contentScrollArea->SetPosition({ 0.f, m_inputArea->GetPosition().y + m_inputArea->GetHeight() + 5.f, 0.f });
	}

	void Console::RefreshContent()
	{
		m_contentText->Clear();

		for (const auto& [color, content] : m_contentLines)
		{
			m_contentText->SetTextColor(color);
			m_contentText->AppendText(content);
			m_contentText->AppendText("\n");
		}

		Layout();
	}
}
