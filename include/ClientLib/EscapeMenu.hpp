// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_ESCAPEMENU_HPP
#define TSOM_CLIENTLIB_ESCAPEMENU_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Widgets/ButtonWidget.hpp>
#include <Nazara/Widgets/Canvas.hpp>
#include <NazaraUtils/Signal.hpp>
#include <vector>

namespace Nz
{
	class BoxLayout;
	class ButtonWidget;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API EscapeMenu : public Nz::BaseWidget
	{
		public:
			EscapeMenu(Nz::BaseWidget* parent);
			EscapeMenu(const EscapeMenu&) = delete;
			EscapeMenu(EscapeMenu&&) = delete;
			~EscapeMenu() = default;

			void AddButton(std::string buttonName, std::function<void()> callback);

			EscapeMenu& operator=(const EscapeMenu&) = delete;
			EscapeMenu& operator=(EscapeMenu&&) = delete;

		private:
			void Layout() override;

			std::vector<Nz::ButtonWidget*> m_buttons;
			Nz::BoxLayout* m_layout;
	};
}

#include <ClientLib/EscapeMenu.inl>

#endif // TSOM_CLIENTLIB_ESCAPEMENU_HPP
