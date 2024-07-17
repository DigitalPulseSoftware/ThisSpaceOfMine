// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CHATBOX_HPP
#define TSOM_CLIENTLIB_CHATBOX_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
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

			void PrintMessage(std::vector<Item> message);

			void SendMessage();

			Chatbox& operator=(const Chatbox&) = delete;
			Chatbox& operator=(Chatbox&&) = delete;

			NazaraSignal(OnChatMessage, const std::string& /*message*/);

		private:
			void Layout() override;
			void Refresh();

			NazaraSlot(Nz::RenderTarget, OnRenderTargetSizeChange, m_onTargetChangeSizeSlot);

			std::vector<std::vector<Item>> m_chatLines;
			Nz::RichTextAreaWidget* m_chatBox;
			Nz::ScrollAreaWidget* m_chatboxScrollArea;
			Nz::TextAreaWidget* m_chatEnteringBox;
	};
}

#include <ClientLib/Chatbox.inl>

#endif // TSOM_CLIENTLIB_CHATBOX_HPP
