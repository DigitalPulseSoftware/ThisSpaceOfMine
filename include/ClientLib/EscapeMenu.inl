// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/EscapeMenu.hpp>

namespace tsom
{
	inline void EscapeMenu::Hide()
	{
		return Show(false);
	}

	inline bool EscapeMenu::IsVisible() const
	{
		return m_backgroundWidget->IsVisible();
	}
}
