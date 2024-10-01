// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::UInt32 EntityClass::FindClientRpc(std::string_view rpcName) const
	{
		auto it = m_clientRpcIndices.find(rpcName);
		if (it == m_clientRpcIndices.end())
			return InvalidIndex;

		return it->second;
	}

	inline Nz::UInt32 EntityClass::FindProperty(std::string_view propertyName) const
	{
		auto it = m_propertyIndices.find(propertyName);
		if (it == m_propertyIndices.end())
			return InvalidIndex;

		return it->second;
	}

	inline auto EntityClass::GetClientRpc(Nz::UInt32 rpcIndex) const -> const RemoteProcedureCall&
	{
		assert(rpcIndex < m_clientRpcs.size());
		return m_clientRpcs[rpcIndex];
	}

	inline Nz::UInt32 EntityClass::GetClientRpcCount() const
	{
		return Nz::UInt32(m_clientRpcs.size());
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

	inline Nz::UInt32 EntityClass::GetPropertyCount() const
	{
		return Nz::UInt32(m_properties.size());
	}
}
