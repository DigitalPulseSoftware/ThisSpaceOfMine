// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Protocol/NetworkStringStore.hpp>

namespace tsom
{
	Packets::NetworkStrings NetworkStringStore::BuildPacket(Nz::UInt32 firstId) const
	{
		Packets::NetworkStrings packet;
		packet.startId = firstId;
		packet.strings.reserve(m_strings.size() - firstId);
		for (Nz::UInt32 i = firstId; i < m_strings.size(); ++i)
			packet.strings.emplace_back(m_strings[i]);

		return packet;
	}

	void NetworkStringStore::FillStore(Nz::UInt32 firstId, std::vector<SecuredString<1024>> strings)
	{
		assert(firstId <= m_strings.size());

		// First, remove all strings with an id over firstId, if any
		for (Nz::UInt32 i = firstId; i < m_strings.size(); ++i)
			m_stringMap.erase(m_strings[i]);

		m_strings.erase(m_strings.begin() + firstId, m_strings.end());

		m_strings.reserve(m_strings.size() - firstId + strings.size());
		for (SecuredString<1024>& str : strings)
			RegisterString(std::move(str).Str());
	}
}
