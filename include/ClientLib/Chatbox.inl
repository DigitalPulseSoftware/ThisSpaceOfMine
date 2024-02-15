// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com) (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

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
