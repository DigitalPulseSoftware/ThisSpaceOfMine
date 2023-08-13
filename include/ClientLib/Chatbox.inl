// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline void Chatbox::Close()
	{
		return Open(false);
	}

	inline bool Chatbox::IsOpen() const
	{
		return m_chatEnteringBox->IsVisible();
	}

	inline bool Chatbox::IsTyping() const
	{
		return m_chatEnteringBox->IsVisible();
	}
}
