// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline auto ConnectionState::GetConnectionInfo() const -> const ConnectionInfo*
	{
		if (!m_connectionInfo.has_value())
			return nullptr;

		return &m_connectionInfo.value();
	}

	inline bool ConnectionState::HasSession() const
	{
		return m_serverSession.has_value();
	}
}
