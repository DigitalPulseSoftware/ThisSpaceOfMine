// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::UInt32 EntityClass::FindProperty(std::string_view propertyName) const
	{
		auto it = m_propertyIndices.find(propertyName);
		if (it == m_propertyIndices.end())
			return InvalidIndex;

		return it->second;
	}

	inline const std::string& EntityClass::GetName() const
	{
		return m_name;
	}

	inline auto EntityClass::GetProperty(Nz::UInt32 propertyIndex) const -> const Property&
	{
		assert(propertyIndex < m_properties.size());
		return m_properties[propertyIndex];
	}
}
