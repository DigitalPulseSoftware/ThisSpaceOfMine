// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ESCAPEMENU_HPP
#define TSOM_CLIENTLIB_ESCAPEMENU_HPP

#include <ClientLib/Export.hpp>
#include <NazaraUtils/Signal.hpp>
#include <Nazara/Renderer/RenderTarget.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/Canvas.hpp>

namespace tsom
{
	class TSOM_CLIENTLIB_API EscapeMenu
	{
		public:
			EscapeMenu(Nz::Canvas* canvas);
			EscapeMenu(const EscapeMenu&) = delete;
			EscapeMenu(EscapeMenu&&) = delete;
			~EscapeMenu();

			inline void Hide();

			inline bool IsVisible() const;

			void Show(bool shouldOpen = true);

			EscapeMenu& operator=(const EscapeMenu&) = delete;
			EscapeMenu& operator=(EscapeMenu&&) = delete;

			NazaraSignal(OnDisconnect, EscapeMenu* /*emitter*/);
			NazaraSignal(OnQuitApp, EscapeMenu* /*emitter*/);

		private:
			void OnBackButtonPressed();

			void Layout();

			Nz::ButtonWidget* m_closeMenuButton;
			Nz::ButtonWidget* m_disconnectButton;
			Nz::ButtonWidget* m_quitAppButton;
			Nz::BaseWidget* m_backgroundWidget;
	};
}

#include <ClientLib/EscapeMenu.inl>

#endif
