// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline EntityClass::EntityClass(std::string name, std::vector<Property> properties, Callbacks callbacks) :
	m_callbacks(std::move(callbacks)),
	m_name(std::move(name)),
	m_properties(std::move(properties))
	{
	}

	inline const std::string& EntityClass::GetName() const
	{
		return m_name;
	}
}
