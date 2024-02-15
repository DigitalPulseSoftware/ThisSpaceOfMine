// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_CHATBOX_HPP
#define TSOM_CLIENTLIB_CHATBOX_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Graphics/RenderTarget.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <Nazara/Widgets/RichTextAreaWidget.hpp>
#include <Nazara/Widgets/ScrollAreaWidget.hpp>
#include <Nazara/Widgets/TextAreaWidget.hpp>
#include <NazaraUtils/Signal.hpp>
#include <variant>

namespace tsom
{
	class TSOM_CLIENTLIB_API Chatbox
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

			Chatbox(Nz::RenderTarget& renderTarget, Nz::Canvas* canvas);
			Chatbox(const Chatbox&) = delete;
			Chatbox(Chatbox&&) = delete;
			~Chatbox();

			void Clear();
			inline void Close();

			inline bool IsOpen() const;
			inline bool IsTyping() const;

			void Open(bool shouldOpen = true);

			void PrintMessage(std::vector<Item> message);

			void SendMessage();

			Chatbox& operator=(const Chatbox&) = delete;
			Chatbox& operator=(Chatbox&&) = delete;

			NazaraSignal(OnChatMessage, const std::string& /*message*/);

		private:
			void OnRenderTargetSizeChange(const Nz::RenderTarget* renderTarget, const Nz::Vector2ui& newSize);
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
