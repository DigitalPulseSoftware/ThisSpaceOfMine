// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

namespace tsom
{
	inline GameConfigFile& GameConfigAppComponent::GetConfig()
	{
		return m_configFile;
	}

	inline const GameConfigFile& GameConfigAppComponent::GetConfig() const
	{
		return m_configFile;
	}
}