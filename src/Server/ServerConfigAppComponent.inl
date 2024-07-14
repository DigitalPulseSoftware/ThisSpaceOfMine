// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline const std::array<Nz::UInt8, 32>& ServerConfigFile::GetConnectionTokenEncryptionKey() const
	{
		return m_connectionTokenEncryptionKey;
	}

	inline ServerConfigFile& ServerConfigAppComponent::GetConfig()
	{
		return m_configFile;
	}

	inline const ServerConfigFile& ServerConfigAppComponent::GetConfig() const
	{
		return m_configFile;
	}
}
