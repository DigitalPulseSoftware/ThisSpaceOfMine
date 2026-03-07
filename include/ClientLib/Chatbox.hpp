// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CHATBOX_HPP
#define TSOM_CLIENTLIB_CHATBOX_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/Timestamp.hpp>
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
	class TSOM_CLIENTLIB_API Chatbox : public Nz::BaseWidget
	{
		public:
			struct ColorItem
			{
				Nz::Color color;
			};

			struct TextItem
			{
				std::string text;
			};

			using Item = std::variant<ColorItem, TextItem>;

			Chatbox(Nz::BaseWidget* parent);
			Chatbox(const Chatbox&) = delete;
			Chatbox(Chatbox&&) = delete;
			~Chatbox() = default;

			void Clear();
			inline void Close();

			bool IsOpen() const;
			bool IsTyping() const;

			void Open(bool shouldOpen = true);

			void PrintMessage(std::vector<Item> items);
			void PrintMessage(std::vector<Item> items, Nz::Time disappearTime);

			void SendMessage();
			void SetFocus();

			void Update();

			Chatbox& operator=(const Chatbox&) = delete;
			Chatbox& operator=(Chatbox&&) = delete;

			NazaraSignal(OnChatMessage, const std::string& /*message*/);

		private:
			void Layout() override;
			void Refresh();

			struct Entry
			{
				std::vector<Item> items;
				Nz::Timestamp disappearTime;
			};

			std::vector<Entry> m_chatEntries;
			Nz::BaseWidget* m_chatboxBackground;
			Nz::RichTextAreaWidget* m_chatboxHistory;
			Nz::ScrollAreaWidget* m_chatboxScrollArea;
			Nz::TextAreaWidget* m_chatEnteringBox;
			Nz::Timestamp m_nextDisappearTime;
	};
}

#include <ClientLib/Chatbox.inl>

#endif // TSOM_CLIENTLIB_CHATBOX_HPP
