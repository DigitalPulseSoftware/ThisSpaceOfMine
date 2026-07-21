// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline ToolBase::ToolBase(GameInterface& gameInterface, std::string toolName) :
	m_toolName(std::move(toolName)),
	m_gameInterface(gameInterface),
	m_isCursorUnlocked(false)
	{
	}

	inline const std::string& ToolBase::GetName() const
	{
		return m_toolName;
	}

	inline bool ToolBase::IsCursorUnlocked() const
	{
		return m_isCursorUnlocked;
	}
}
