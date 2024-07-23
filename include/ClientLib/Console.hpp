// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CONSOLE_HPP
#define TSOM_CLIENTLIB_CONSOLE_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Widgets/BaseWidget.hpp>
#include <variant>

namespace Nz
{
	class RichTextAreaWidget;
	class ScrollAreaWidget;
	class TextAreaWidget;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API Console : public Nz::BaseWidget
	{
		public:
			Console(Nz::BaseWidget* parent);
			Console(const Console&) = delete;
			Console(Console&&) = delete;
			~Console() = default;

			void Clear();

			const Nz::Color& GetBackgroundColor() const;
			unsigned int GetCharacterSize() const;

			void PrintMessage(std::string message, const Nz::Color& color = Nz::Color::White());

			void SetBackgroundColor(const Nz::Color& backgroundColor);
			void SetCharacterSize(unsigned int size);

			Console& operator=(const Console&) = delete;
			Console& operator=(Console&&) = delete;

			NazaraSignal(OnCommand, std::string_view /*command*/);

			static constexpr std::string_view CommandPrefix = "> ";

		private:
			void Layout() override;
			void RefreshContent();

			struct Line
			{
				Nz::Color color;
				std::string text;
			};

			std::size_t m_historyPosition;
			std::vector<std::string> m_commandHistory;
			std::vector<Line> m_contentLines;
			Nz::RichTextAreaWidget* m_contentText;
			Nz::ScrollAreaWidget* m_contentScrollArea;
			Nz::TextAreaWidget* m_inputArea;
			unsigned int m_maxHistoryLines;
	};
}

#include <ClientLib/Console.inl>

#endif // TSOM_CLIENTLIB_CONSOLE_HPP
